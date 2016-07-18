#include <gmi_mesh.h>
#include <apfMDS.h>
#include <apfMesh2.h>
#include <apfConvert.h>
#include <apf.h>
#include <PCU.h>
#include <cassert>
#include <cstdlib>

#define INTERIORTAG 0
#define BOTTOMTAG 1
#define TOPTAG 2
#define PERIMETERTAG 3
#define BOTTOM_PERIMETERTAG 4
#define TOP_PERIMETERTAG 5
#define TOPFACE 1
#define BOTTOMFACE 2
#define PERIMETERFACE 3

unsigned getElmType(int numVtxPerElm) {
  if (numVtxPerElm == 4) {
    return apf::Mesh::TET;
  } else if (numVtxPerElm == 6) {
    return apf::Mesh::PRISM;
  } else {
    fprintf(stderr, "unknown element type in %s\n", __func__);
    exit(EXIT_FAILURE);
  }
}

void readHeader(FILE* f, unsigned& nodes, unsigned& elms, unsigned& numVtxPerElm) {
  gmi_fscanf(f, 3, "%d %d %d", &nodes, &elms, &numVtxPerElm);
}

void readCoords(FILE* f, unsigned numvtx, double* coordinates) {
  const double huge = 1024*1024;
  double min[3] = {huge,huge,huge};
  double max[3] = {-huge,-huge,-huge};
  for(unsigned i=0; i<numvtx; i++) {
    for(unsigned j=0; j<3; j++) {
      double pos = 0;
      gmi_fscanf(f, 1, "%lf", &pos);
      coordinates[i*3+j] = pos;
      if( pos < min[j] )
        min[j] = pos;
      if( pos > max[j] )
        max[j] = pos;
    }
  }
  char d[3] = {'x','y','z'};
  fprintf(stderr, "mesh bounding box:\n");
  for(unsigned i=0; i<3; i++)
    fprintf(stderr, "%c %lf %lf \n", d[i], min[i], max[i]);
}

void readElements(FILE* f, unsigned numelms, int numVtxPerElm,
    unsigned numVerts, int* elements) {
  unsigned i;
  std::map<int, int> count;
  for (i = 0; i < numelms*numVtxPerElm; i++) {
    int vtxid;
    gmi_fscanf(f, 1, "%u", &vtxid);
    elements[i] = --vtxid; //export from matlab using 1-based indices
    count[elements[i]]++;
  }
  assert(count.size() == numVerts);
}

struct MeshInfo {
  double* coords;
  int* elements;
  unsigned elementType;
  unsigned numVerts;
  unsigned numElms;
  unsigned numVtxPerElm;
};

void readMesh(const char* meshfilename, MeshInfo& mesh) {
  FILE* f = fopen(meshfilename, "r");
  assert(f);
  readHeader(f,mesh.numVerts,mesh.numElms,mesh.numVtxPerElm);
  fprintf(stderr, "numVerts %u numElms %u numVtxPerElm %u\n",
      mesh.numVerts, mesh.numElms, mesh.numVtxPerElm);
  mesh.coords = new double[mesh.numVerts*3];
  readCoords(f, mesh.numVerts, mesh.coords);
  mesh.elements = new int [mesh.numElms*mesh.numVtxPerElm];
  readElements(f, mesh.numElms, mesh.numVtxPerElm, mesh.numVerts, mesh.elements);
  mesh.elementType = getElmType(mesh.numVtxPerElm);
  fclose(f);
}


bool isClassifiedOnBoundary(apf::Mesh2* mesh, apf::MeshEntity* face) {
  int numAdjElms = mesh->countUpward(face);
  assert( numAdjElms > 0 );
  if( numAdjElms == 2 )
    return false; // both regions exist -> interior
  else
    return true; // only one region -> exterior
}

/** \brief just get the first geometric model region - hack
*/
apf::ModelEntity* getMdlRgn(gmi_model* model) {
  gmi_iter* it = gmi_begin(model, 3);
  gmi_ent* rgn = gmi_next(model, it);
  assert(rgn);
  gmi_end(model, it);
  return (apf::ModelEntity*)rgn;
}

apf::ModelEntity* getMdlFace(apf::Mesh2* mesh, int tag) {
  apf::ModelEntity* face = mesh->findModelEntity(2,tag);
  assert(face);
  return face;
}

/** \brief brute force serach to find the intersection
 * \details models don't have that many edges bounding each face.... right???
 */
gmi_ent* getCommonEdge(gmi_set* a, gmi_set* b) {
  gmi_ent* match = NULL;
  for(int i=0; i<a->n; i++) {
    for(int j=0; j<b->n; j++) {
      if( a->e[i] == b->e[j] ) {
        assert(!match);
        match = a->e[i];
      }
    }
  }
  assert(match);
  return match;
}

/** \brief get the model edge that bounds the model faces with the given tags
*/
apf::ModelEntity* getMdlEdge(apf::Mesh2* mesh, int faceTagA, int faceTagB) {
  gmi_model* model = mesh->getModel();
  gmi_ent* faceA = (gmi_ent*) getMdlFace(mesh,faceTagA);
  gmi_set* faceAedges = gmi_adjacent(model, faceA, 1);
  assert(faceAedges->n);
  gmi_ent* faceB = (gmi_ent*) getMdlFace(mesh,faceTagB);
  gmi_set* faceBedges = gmi_adjacent(model, faceB, 1);
  assert(faceBedges->n);
  gmi_ent* edge = getCommonEdge(faceAedges,faceBedges);
  gmi_free_set(faceAedges);
  gmi_free_set(faceBedges);
  return (apf::ModelEntity*) edge;
}

/* returns -1 for an interior face
 *          0 for a boundary face with at least one vertex with an interior tag
 *          1 for a marked boundary face
 */
int setModelClassification(gmi_model* model,
    apf::Mesh2* mesh,
    std::map<int,int> vtx_type_num,
    apf::MeshEntity* face,
    std::map<int,int>& faceClass) {
  if( ! isClassifiedOnBoundary(mesh,face) ) {
    mesh->setModelEntity(face,getMdlRgn(model));
    return -1;
  }
  //this is a boundary face
  assert(vtx_type_num.at(INTERIORTAG) == 0);
  //we only need to mark vertex that in on the boundary
  if(vtx_type_num[PERIMETERTAG]!=0) {
    //if perimeter point exist it's on the perimeter
    faceClass[PERIMETERFACE]++;
    mesh->setModelEntity(face,getMdlFace(mesh,PERIMETERFACE));
  }
  else {
    if((vtx_type_num[BOTTOMTAG] + vtx_type_num[BOTTOM_PERIMETERTAG]) == 3) {
      /*include vtx_type_num[BOTTOMTAG]=3;
        vtx_type_num[BOTTOMTAG]=2, vtx_type_num[BOTTOM_PERIMETERTAG])=1;
        vtx_type_num[BOTTOMTAG]=1, vtx_type_num[BOTTOM_PERIMETERTAG])=2;
        vtx_type_num[BOTTOM_PERIMETERTAG])=3;*/
      faceClass[BOTTOMFACE]++;
      mesh->setModelEntity(face,getMdlFace(mesh,BOTTOMFACE));
    }
    else {
      if((vtx_type_num[TOPTAG] + vtx_type_num[TOP_PERIMETERTAG]) == 3) {
        /*include vtx_type_num[TOPTAG]=3;
          vtx_type_num[TOPTAG]=2, vtx_type_num[TOP_PERIMETERTAG])=1;
          vtx_type_num[TOPTAG]=1, vtx_type_num[TOP_PERIMETERTAG])=2;
          vtx_type_num[BOTTOM_PERIMETERTAG])=3;*/
        faceClass[TOPFACE]++;
        mesh->setModelEntity(face,getMdlFace(mesh,TOPFACE));
      }
      else {
        faceClass[PERIMETERFACE]++;
        mesh->setModelEntity(face,getMdlFace(mesh,PERIMETERFACE));
      }
    }
  }
  return 1;
}

void setFaceClassification(gmi_model* model, apf::Mesh2* mesh, apf::MeshTag* vtxType) {
  int numbdryfaces = 0;
  int markedfaces = 0;
  int skippedfaces = 0;
  std::map<int,int> faceClass;

  apf::MeshIterator* it = mesh->begin(2);
  apf::MeshEntity* face;
  while( (face = mesh->iterate(it)) ) {
    apf::Downward verts;
    int n = mesh->getDownward(face, 0, verts);
    assert(n);

    std::map<int, int> vtx_type_num;
    for(int i=INTERIORTAG; i<=TOP_PERIMETERTAG; i++)
      vtx_type_num[i] = 0;
    for(int i=0; i<3; i++){
      int value;
      mesh->getIntTag(verts[i], vtxType, &value);
      assert(value >= INTERIORTAG && value <= TOP_PERIMETERTAG);
      vtx_type_num[value]++;
    }
    int counttaggedvtx = 0;
    for(int i=INTERIORTAG; i<=TOP_PERIMETERTAG; i++)
      counttaggedvtx += vtx_type_num[i];
    assert(counttaggedvtx==3);
    int isSet = setModelClassification(model, mesh, vtx_type_num, face, faceClass);
    if( isSet == 1 )
      markedfaces++;
    if(isSet == 0)
      skippedfaces++;
    if( isSet == 0 || isSet == 1 )
      numbdryfaces++;
  }
  mesh->end(it);

  assert(!skippedfaces);

  int totmarkedfaces=0;
  for(int i=1; i<4; i++)
    totmarkedfaces += faceClass[i];
  assert(numbdryfaces == totmarkedfaces);
}

/** \brief set the mesh region classification
  \details hacked to set the classification to the same geometric model region
*/
void setRgnClassification(gmi_model* model, apf::Mesh2* mesh) {
  apf::ModelEntity* mdlRgn = getMdlRgn(model);
  apf::MeshIterator* it = mesh->begin(3);
  apf::MeshEntity* rgn;
  while( (rgn = mesh->iterate(it)) )
    mesh->setModelEntity(rgn,mdlRgn);
  mesh->end(it);
}

void getMeshEdgeTags(apf::Mesh2* mesh, apf::MeshEntity* edge,
    apf::MeshTag* t, int* tags) {
  apf::Downward verts;
  int n = mesh->getDownward(edge, 0, verts);
  assert(n == 2);

  for(int i=0; i<2; i++){
    int value;
    mesh->getIntTag(verts[i], t, &value);
    assert(value >= INTERIORTAG && value <= TOP_PERIMETERTAG);
    tags[i] = value;
  }
}

/** \brief set the mesh region classification
  \details hack - interior edges have classification set to the
  same geometric model region
*/
void setEdgeClassification(gmi_model* model, apf::Mesh2* mesh) {
  apf::ModelEntity* mdlRgn = getMdlRgn(model);
  apf::ModelEntity* mdlBotEdge = getMdlEdge(mesh,BOTTOMFACE,PERIMETERFACE);
  apf::ModelEntity* mdlTopEdge = getMdlEdge(mesh,TOPFACE,PERIMETERFACE);
  apf::ModelEntity* mdlTopFace = getMdlFace(mesh,TOPFACE);
  apf::ModelEntity* mdlBotFace = getMdlFace(mesh,BOTTOMFACE);
  apf::ModelEntity* mdlPerFace = getMdlFace(mesh,PERIMETERFACE);
  apf::MeshIterator* it = mesh->begin(1);
  apf::MeshEntity* edge;
  while( (edge = mesh->iterate(it)) ) {
    apf::Up adj_faces;
    mesh->getUp(edge, adj_faces);
    std::set<apf::ModelEntity*> adj_mdl_faces;
    for (int i = 0; i < adj_faces.n; ++i) {
      apf::MeshEntity* adj_face = adj_faces.e[i];
      apf::ModelEntity* face_class = mesh->toModel(adj_face);
      if (mesh->getModelType(face_class) == 2)
        adj_mdl_faces.insert(face_class);
    }
    // classified on bottom perimeter
    if(adj_mdl_faces.count(mdlBotFace) && adj_mdl_faces.count(mdlPerFace))
      mesh->setModelEntity(edge,mdlBotEdge);
    // classified on top perimeter
    else if(adj_mdl_faces.count(mdlTopFace) && adj_mdl_faces.count(mdlPerFace))
      mesh->setModelEntity(edge,mdlTopEdge);
    // classified on top face
    else if(adj_mdl_faces.count(mdlTopFace))
      mesh->setModelEntity(edge,mdlTopFace);
    // classified on bottom face
    else if(adj_mdl_faces.count(mdlBotFace))
      mesh->setModelEntity(edge,mdlBotFace);
    // classified on perimeter face
    else if(adj_mdl_faces.count(mdlPerFace))
      mesh->setModelEntity(edge,mdlPerFace);
    // classified on model region
    else
      mesh->setModelEntity(edge,mdlRgn);
  }
  mesh->end(it);
}

void setVtxClassification(gmi_model* model, apf::Mesh2* mesh, apf::MeshTag* t) {
  apf::ModelEntity* mdlRgn = getMdlRgn(model);
  apf::ModelEntity* mdlBotEdge = getMdlEdge(mesh,BOTTOMFACE,PERIMETERFACE);
  apf::ModelEntity* mdlTopEdge = getMdlEdge(mesh,TOPFACE,PERIMETERFACE);
  apf::ModelEntity* mdlTopFace = getMdlFace(mesh,TOPFACE);
  apf::ModelEntity* mdlBotFace = getMdlFace(mesh,BOTTOMFACE);
  apf::ModelEntity* mdlPerFace = getMdlFace(mesh,PERIMETERFACE);
  apf::MeshIterator* it = mesh->begin(0);
  apf::MeshEntity* vtx;
  while( (vtx = mesh->iterate(it)) ) {
    int tag = -1;
    mesh->getIntTag(vtx, t, &tag);
    if( tag == INTERIORTAG )
      mesh->setModelEntity(vtx,mdlRgn);
    // classified on bottom perimeter
    else if( tag == BOTTOM_PERIMETERTAG )
      mesh->setModelEntity(vtx,mdlBotEdge);
    // classified on top perimeter
    else if( tag == TOP_PERIMETERTAG )
      mesh->setModelEntity(vtx,mdlTopEdge);
    // classified on top face
    else if( tag == TOPTAG )
      mesh->setModelEntity(vtx,mdlTopFace);
    // classified on bottom face
    else if( tag == BOTTOMTAG )
      mesh->setModelEntity(vtx,mdlBotFace);
    // classified on perimeter face
    else if( tag == PERIMETERTAG )
      mesh->setModelEntity(vtx,mdlPerFace);
    else {
      fprintf(stderr, "classification of a vertex fell through the conditional...exiting\n");
      exit(EXIT_FAILURE);
    }
  }
  mesh->end(it);
}

void setClassification(gmi_model* model, apf::Mesh2* mesh, apf::MeshTag* t) {
  setRgnClassification(model,mesh);
  setFaceClassification(model,mesh,t);
  setEdgeClassification(model,mesh);
  setVtxClassification(model,mesh,t);
  mesh->acceptChanges();
}

int* readIntArray(const char* fname, unsigned len) {
  FILE* f = fopen(fname, "r");
  assert(f);
  unsigned n;
  gmi_fscanf(f, 1, "%u", &n);
  assert( n == len );
  int* data = new int[len];
  for(unsigned i = 0; i< len; i++)
    gmi_fscanf(f, 1, "%d", &data[i]);
  fclose(f);
  return data;
}

double* readScalarArray(const char* fname, unsigned len) {
  FILE* f = fopen(fname, "r");
  assert(f);
  unsigned n;
  gmi_fscanf(f, 1, "%u", &n);
  assert( n == len );
  double* data = new double[len];
  for(unsigned i = 0; i< len; i++)
    gmi_fscanf(f, 1, "%lf", &data[i]);
  fclose(f);
  return data;
}

std::string getName(const char* fname) {
  using std::string;
  std::string in(fname);
  if( in.find("vertex_type") != string::npos ) {
    return string("vertex_type");
  } else if( in.find("basal_friction") != string::npos ) {
    return string("basal_friction");
  } else if( in.find("temperature") != string::npos ) {
    return string("temperature");
  } else if( in.find("surface_elevation") != string::npos ) {
    return string("surface_height");
  } else if( in.find("solution_x") != string::npos ) {
    return string("solution_x");
  } else if( in.find("solution_y") != string::npos ) {
    return string("solution_y");
  } else {
    fprintf(stderr, "unknown field name in %s\n", __func__);
    exit(EXIT_FAILURE);
  }
}

apf::MeshTag* attachVtxTag(apf::Mesh2* mesh, const char* fname,
    apf::GlobalToVert& vtxMap) {
  unsigned numverts = mesh->count(0);
  std::string fldname = getName(fname);
  fprintf(stderr, "attaching %s\n", fldname.c_str());
  int* arr = readIntArray(fname, numverts);
  apf::MeshTag* tag = mesh->createIntTag(fldname.c_str(), 1);
  for(unsigned i=0; i<numverts; i++)
    mesh->setIntTag(vtxMap[i], tag, &(arr[i]));
  delete [] arr;
  return tag;
}

/** \brief attach a field to the vertices
 *  \detail all the fields being attached need to exist with the mesh so the
 *  field pointer is not returned
 */
void attachVtxField(apf::Mesh2* mesh, const char* fname,
    apf::GlobalToVert& vtxMap) {
  std::string fldname = getName(fname);
  unsigned numverts = mesh->count(0);
  fprintf(stderr, "attaching %s\n", fldname.c_str());
  double* arr = readScalarArray(fname, numverts);
  apf::Field* fld = apf::createFieldOn(mesh,fldname.c_str(),apf::SCALAR);
  for(unsigned i=0; i<numverts; i++)
    apf::setScalar(fld,vtxMap[i],0,arr[i]);
  delete [] arr;
}

void mergeSolutionFields(apf::Mesh2* mesh) {
  apf::Field* x = mesh->findField("solution_x");
  apf::Field* y = mesh->findField("solution_y");
  apf::Field* xy = apf::createPackedField(mesh,"Solution",2);
  apf::MeshIterator* it = mesh->begin(0);
  apf::MeshEntity* vtx;
  while( (vtx = mesh->iterate(it)) ) {
    double v[2] = {0,0};
    v[0] = apf::getScalar(x,vtx,0);
    v[1] = apf::getScalar(y,vtx,0);
    apf::setComponents(xy, vtx, 0, v);
  }
  mesh->end(it);
  apf::destroyField(x);
  apf::destroyField(y);
}

void detachVtxTag(apf::Mesh2* mesh, apf::MeshTag* t) {
  apf::MeshIterator* it = mesh->begin(0);
  apf::MeshEntity* vtx;
  while( (vtx = mesh->iterate(it)) )
    mesh->removeTag(vtx,t);
  mesh->end(it);
  mesh->destroyTag(t);
}

int main(int argc, char** argv)
{
  if( argc != 10 ) {
    printf("Usage: %s <model .dmg> <ascii mesh .ascii> "
        "<vertex classification field .ascii> "
        "<basal friction field .ascii> "
        "<temperature field .ascii> "
        "<surface elevation field .ascii> "
        "<solution_x field .ascii> "
        "<solution_y field .ascii> "
        "<output mesh>\n",
        argv[0]);
    return 0;
  }

  MPI_Init(&argc,&argv);
  PCU_Comm_Init();
  gmi_register_mesh();

  gmi_model* model = gmi_load(argv[1]);

  MeshInfo m;
  readMesh(argv[2],m);

  const int dim = 3;
  apf::Mesh2* mesh = apf::makeEmptyMdsMesh(model, dim, false);
  apf::GlobalToVert outMap;
  apf::construct(mesh, m.elements, m.numElms, m.elementType, outMap);
  delete [] m.elements;
  apf::alignMdsRemotes(mesh);
  apf::deriveMdsModel(mesh);
  apf::setCoords(mesh, m.coords, m.numVerts, outMap);
  delete [] m.coords;
  apf::MeshTag* vtxClass = attachVtxTag(mesh,argv[3],outMap);
  setClassification(model,mesh,vtxClass);
  detachVtxTag(mesh,vtxClass);
  mesh->verify();
  for(int i=4; i<argc-1; i++)
    attachVtxField(mesh,argv[i],outMap);
  mergeSolutionFields(mesh);
  outMap.clear();
  mesh->writeNative(argv[9]);
  apf::writeVtkFiles("rendered",mesh);

  mesh->destroyNative();
  apf::destroyMesh(mesh);
  PCU_Comm_Free();
  MPI_Finalize();
}