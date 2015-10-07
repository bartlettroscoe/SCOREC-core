/*
 * Copyright 2015 Scientific Computation Research Center
 *
 * This work is open source software, licensed under the terms of the
 * BSD license as described in the LICENSE file in the top-level directory.
 */

#include "crvBezier.h"
#include "crvTables.h"
#include <cassert>

namespace crv {

void getBezierNodeXi(int type, int P, int node, apf::Vector3& xi)
{
  static double eP2[1] = {0.0};
  static double eP3[2] = {-0.4503914,0.4503914};
  static double eP4[3] = {-0.6612048,0.0,0.6612048};
  static double eP5[4] = {-0.7732854,-0.2863522,0.2863522,0.7732854};
  static double eP6[5] = {-0.8388042,-0.469821,0.0,
      0.469821,0.8388042};
  static double* edgePoints[6] =
  {eP2, eP2, eP3, eP4, eP5, eP6 };
  static apf::Vector3 nodes5[6] =
  {apf::Vector3(0.15251715,0.15251715,0.6949657),
   apf::Vector3(0.4168658,0.1662684,0.4168658),
   apf::Vector3(0.6949657,0.15251715,0.15251715),
   apf::Vector3(0.1662684,0.4168658,0.4168658),
   apf::Vector3(0.4168658,0.4168658,0.1662684),
   apf::Vector3(0.15251715,0.6949657,0.15251715)};
  static apf::Vector3 nodes6[10] =
  {apf::Vector3(0.10971385,0.10971385,0.7805723),
   apf::Vector3(0.3157892,0.1256031,0.5586077),
   apf::Vector3(0.5586077,0.1256031,0.3157892),
   apf::Vector3(0.7805723,0.10971385,0.10971385),
   apf::Vector3(0.1256031,0.3157892,0.5586077),
   apf::Vector3(1./3.,1./3.,1./3.),
   apf::Vector3(0.5586077,0.3157892,0.1256031),
   apf::Vector3(0.1256031,0.5586077,0.3157892),
   apf::Vector3(0.3157892,0.5586077,0.1256031),
   apf::Vector3(0.10971385,0.7805723,0.10971385)};

  switch (type) {
  case apf::Mesh::EDGE:
    xi[0] = edgePoints[P-1][node];
    break;
  case apf::Mesh::TRIANGLE:
    // technically only two of these numbers are needed
    switch (P) {
    case 1:
    case 2:
      fail("expected P >= 3");
    case 3:
      xi = apf::Vector3(1./3.,1./3.,1./3.);
      break;
    case 4:
      xi[(node+2) % 3] = 0.5582239;
      xi[(node+0) % 3] = 0.22088805;
      xi[(node+1) % 3] = 0.22088805;
      break;
    case 5:
      xi = nodes5[node];
      break;
    case 6:
      xi = nodes6[node];
      break;
    }
    break;
  case apf::Mesh::TET:
    switch (P) {
    case 1:
    case 2:
    case 3:
      fail("expected P == 4");
    case 4:
      xi = apf::Vector3(0.25,0.25,0.25);
      break;
    case 5:
    case 6:
      fail("expected P == 4");
    }
    break;
    default:
      xi.zero();
      break;
  }
}

static void getBezierEdgeTransform(int P, apf::NewArray<double> & c)
{
  double e2[3] = {-0.5,-0.5,2};
  double e3[8] = {
      -1.00596379148431,0.333333333333333,2.69317845753742,-1.02054799938644,
      0.333333333333333,-1.00596379148431,-1.02054799938644,2.69317845753742};
  double e4[15] = {
      -1.52680420766155,-0.25,3.37567603341243,-1.28732567375034,
      0.688453847999458,1.01786947177436,1.01786947177436,-2.70941992094126,4.38310089833379,
      -2.70941992094126,-0.25,-1.52680420766155,0.688453847999459,-1.28732567375034,
      3.37567603341243};
  double e5[24] = {
      -2.06136018481524,0.2,4.05936188849008,-1.52513018373641,
      0.846118038541145,-0.518989558479574,2.07392858296555,-1.03068009240762,
      -5.04403528329688,6.4850808350761,-4.1256786562572,2.64138461392004,
      -1.03068009240762,2.07392858296555,2.64138461392004,-4.1256786562572,
      6.48508083507611,-5.04403528329688,0.2,-2.06136018481524,-0.518989558479574,
      0.846118038541145,-1.52513018373641,4.05936188849008};
  double e6[35] = {
      -2.60465921875445,-0.166666666666667,4.74317259573776,-1.74847211573074,
      0.99151406014263,-0.630691218757935,0.415802564029398,3.50945493261743,
      1.04186368750178,-8.00777336834505,8.99109434126308,
      -5.80152934540333,3.85156704410589,-2.5846772917398,-2.63209119946307,
      -2.63209119946307,6.39664544713349,-8.91824703868012,
      11.3073855820194,-8.91824703868012,6.39664544713349,1.04186368750178,
      3.50945493261743,-2.5846772917398,3.85156704410589,
      -5.80152934540333,8.99109434126308,-8.00777336834505,-0.166666666666667,
      -2.60465921875445,0.415802564029398,-0.630691218757935,
      0.991514060142631,-1.74847211573074,4.74317259573777};

  double* table[5] = {
      e2,e3,e4,e5,e6};

  int nb = P-1;
  int ni = P+1;
  c.allocate(ni*nb);
  for( int i = 0; i < nb; ++i)
    for( int j = 0; j < ni; ++j)
      c[i*ni+j] = table[P-2][i*ni+j];
}

static void getBezierTriangleTransform(int P, apf::NewArray<double> & c)
{
  double f3[10] = {
      0.5059637914843115,0.5059637914843116,0.5059637914843119,-0.8363152290754889,
      -0.8363152290754891,-0.8363152290754891,-0.8363152290754897,-0.8363152290754891,
      -0.8363152290754898,4.5};
  double f4[45] = {
      1.473866405971784,-0.4873245458152482,-0.4873245458152483,-2.157170326583087,
      -0.7895371825786641,1.002268609355065,0.4569922360188257,0.4160673296503333,
      0.4569922360188257,1.002268609355065,-0.7895371825786643,-2.157170326583087,
      7.066481928037422,-2.00343662222666,-2.00343662222666,
      -0.4873245458152483,1.473866405971784,-0.4873245458152482,1.002268609355065,
      -0.7895371825786646,-2.157170326583087,-2.157170326583087,-0.7895371825786643,
      1.002268609355065,0.4569922360188255,0.4160673296503333,0.4569922360188255,
      -2.00343662222666,7.066481928037421,-2.00343662222666,
      -0.4873245458152483,-0.4873245458152481,1.473866405971784,0.4569922360188258,
      0.4160673296503332,0.4569922360188255,1.002268609355065,-0.7895371825786646,
      -2.157170326583087,-2.157170326583088,-0.7895371825786643,1.002268609355065,
      -2.00343662222666,-2.00343662222666,7.066481928037422};
  double f5[126] = {
      2.955509375394109,0.4850633218816852,0.4850633218816842,-4.015153971419449,
      -0.6339185304220426,1.140973028173455,-1.041278651590121,-0.3192965376117991,
      -0.2178043751582104,-0.2178043751582106,-0.3192965376117977,-1.041278651590119,
      1.140973028173454,-0.6339185304220449,-4.015153971419449,10.16896081482358,
      -3.086925777444235,1.177507607441265,-3.086925777444229,0.8971975820812144,
      1.177507607441261,
      -1.879829564696171,-1.879829564696172,0.5695483481139592,3.558924203285689,
      -2.437779912677449,-2.437779912677447,3.558924203285689,1.772590081128005,
      0.9751122788728834,0.114906885694974,-0.8337418650640221,-0.8337418650640223,
      0.1149068856949739,0.975112278872884,1.772590081128002,-6.137619043989808,
      12.47852594090007,-6.137619043989812,-2.280366067778762,-2.280366067778761,
      2.247531721435297,
      0.4850633218816853,2.955509375394109,0.4850633218816858,-1.041278651590121,
      1.140973028173455,-0.6339185304220443,-4.015153971419449,-4.01515397141945,
      -0.6339185304220427,1.140973028173454,-1.041278651590121,-0.3192965376118004,
      -0.2178043751582095,-0.2178043751582098,-0.3192965376117998,1.177507607441266,
      -3.086925777444232,10.16896081482358,0.8971975820812099,-3.086925777444233,
      1.177507607441266,
      -1.879829564696168,0.5695483481139586,-1.879829564696167,1.772590081128001,
      0.9751122788728822,0.1149068856949737,-0.8337418650640216,-0.8337418650640215,
      0.1149068856949736,0.9751122788728837,1.772590081127999,3.558924203285684,
      -2.437779912677445,-2.437779912677444,3.558924203285684,-6.137619043989801,
      -2.280366067778758,2.247531721435295,12.47852594090005,-2.28036606777876,
      -6.1376190439898,
      0.5695483481139563,-1.87982956469617,-1.879829564696174,-0.8337418650640195,
      0.114906885694975,0.9751122788728848,1.772590081128002,3.55892420328569,
      -2.437779912677449,-2.437779912677447,3.558924203285693,1.772590081128009,
      0.9751122788728801,0.1149068856949706,-0.8337418650640169,2.247531721435288,
      -2.280366067778765,-6.137619043989809,-2.280366067778744,12.47852594090007,
      -6.137619043989821,
      0.4850633218816849,0.485063321881685,2.955509375394111,-0.3192965376117988,
      -0.2178043751582104,-0.2178043751582108,-0.3192965376117985,-1.041278651590121,
      1.140973028173456,-0.6339185304220445,-4.01515397141945,-4.015153971419451,
      -0.6339185304220428,1.140973028173455,-1.04127865159012,1.177507607441264,
      0.897197582081215,1.177507607441264,-3.086925777444235,-3.086925777444234,
      10.16896081482358};
  double f6[280] = {
      4.990795106388402,-0.4798837984147304,-0.4798837984147311,-6.458245423578353,
      -0.3639631629214184,1.223365632584435,-1.237281132017871,1.048637951819914,
      0.2399023612066779,0.1476405704559403,0.09371675625976672,0.1476405704559403,
      0.2399023612066771,1.048637951819915,-1.237281132017874,1.22336563258444,
      -0.3639631629214241,-6.458245423578351,13.89283735202937,-4.276280903149718,
      1.808916611395359,-0.815238923186442,-4.276280903149713,1.327623829722842,
      -0.4900563063939837,1.808916611395356,-0.4900563063939835,-0.81523892318644,
      -4.689458981957332,2.303409358216716,-0.6686820957216246,8.401259643873194,
      -5.32764003589759,-2.380733253738298,4.596086431435173,-4.676887162735884,
      -1.613859854887052,-0.8313011663168198,-0.353269505070007,0.01692167342425344,
      0.783807813660766,1.115432125134819,-0.6396258416621945,-0.4063403163659929,
      1.63350774282936,4.420179509817762,-13.10100871536224,20.0696850984568,
      -10.70127780752898,5.237779423093969,-2.093285349716033,-4.18119984946875,
      2.406990467529693,3.071067981878368,0.7247013815918737,-2.116258714513938,
      2.303409358216713,-4.689458981957323,-0.6686820957216264,-4.676887162735882,
      4.596086431435173,-2.380733253738303,-5.327640035897582,8.40125964387318,
      4.420179509817754,1.633507742829359,-0.4063403163659948,-0.6396258416621916,
      1.115432125134818,0.7838078136607716,0.01692167342424611,-0.3532695050700018,
      -0.8313011663168233,-1.613859854887048,5.237779423093964,-10.70127780752897,
      20.06968509845678,-13.10100871536223,2.40699046752969,-4.181199849468755,
      -2.093285349716027,0.7247013815918821,3.071067981878365,-2.116258714513939,
      -0.4798837984147314,4.990795106388401,-0.4798837984147301,1.048637951819916,
      -1.237281132017876,1.22336563258444,-0.3639631629214218,-6.458245423578349,
      -6.458245423578349,-0.3639631629214216,1.223365632584435,-1.237281132017871,
      1.048637951819913,0.2399023612066772,0.1476405704559403,0.09371675625976489,
      0.1476405704559418,0.2399023612066772,-0.8152389231864428,1.808916611395362,
      -4.276280903149718,13.89283735202936,-0.4900563063939837,1.327623829722837,
      -4.276280903149709,-0.4900563063939801,1.808916611395357,-0.8152389231864411,
      -4.689458981957326,-0.6686820957216243,2.303409358216713,4.420179509817762,
      1.633507742829353,-0.4063403163659914,-0.6396258416621946,1.115432125134818,
      0.7838078136607659,0.01692167342425165,-0.3532695050700053,-0.8313011663168223,
      -1.613859854887046,-4.676887162735882,4.596086431435175,-2.380733253738309,
      -5.327640035897575,8.401259643873182,-13.10100871536224,-2.093285349716022,
      3.071067981878363,-2.116258714513934,20.06968509845678,-4.181199849468753,
      0.7247013815918747,-10.70127780752897,2.406990467529694,5.237779423093959,
      2.740410900541122,2.740410900541103,2.740410900541114,-3.896719679725162,
      0.8525687056196811,2.629196981978401,0.8525687056196632,-3.896719679725137,
      -3.896719679725126,0.8525687056196491,2.629196981978421,0.8525687056196478,
      -3.896719679725137,-3.896719679725156,0.8525687056196771,2.629196981978398,
      0.8525687056196591,-3.89671967972515,9.218530840490791,-7.999447648758395,
      -7.999447648758339,9.218530840490757,-7.999447648758336,23.49717556815216,
      -7.999447648758354,-7.999447648758379,-7.999447648758353,9.218530840490786,
      -0.6686820957216238,-4.689458981957319,2.303409358216712,1.115432125134813,
      -0.6396258416621886,-0.4063403163659943,1.633507742829352,4.420179509817753,
      8.401259643873173,-5.327640035897578,-2.380733253738297,4.59608643143517,
      -4.676887162735879,-1.61385985488705,-0.8313011663168175,-0.35326950507,
      0.01692167342424691,0.7838078136607683,-2.116258714513931,3.071067981878351,
      -2.093285349716015,-13.10100871536222,0.7247013815918777,-4.181199849468735,
      20.06968509845676,2.406990467529674,-10.70127780752897,5.237779423093965,
      2.303409358216709,-0.6686820957216228,-4.689458981957325,-1.613859854887044,
      -0.8313011663168245,-0.3532695050700039,0.01692167342424997,0.7838078136607665,
      1.115432125134813,-0.6396258416621888,-0.406340316365999,1.633507742829364,
      4.420179509817753,8.401259643873189,-5.327640035897586,-2.380733253738296,
      4.596086431435168,-4.676887162735876,5.237779423093958,2.406990467529699,
      0.724701381591874,-2.116258714513933,-10.70127780752898,-4.181199849468753,
      3.071067981878364,20.06968509845678,-2.093285349716028,-13.10100871536224,
      -0.6686820957216243,2.303409358216713,-4.689458981957323,0.7838078136607708,
      0.01692167342424194,-0.353269505069999,-0.8313011663168179,-1.613859854887048,
      -4.676887162735883,4.596086431435175,-2.380733253738307,-5.327640035897575,
      8.401259643873177,4.420179509817762,1.633507742829345,-0.4063403163659917,
      -0.6396258416621861,1.115432125134812,-2.11625871451393,0.724701381591889,
      2.40699046752967,5.237779423093964,3.07106798187834,-4.181199849468741,
      -10.70127780752897,-2.093285349716002,20.06968509845677,-13.10100871536223,
      -0.4798837984147306,-0.4798837984147314,4.990795106388401,0.2399023612066765,
      0.1476405704559429,0.09371675625976505,0.1476405704559399,0.2399023612066777,
      1.048637951819916,-1.237281132017875,1.223365632584441,-0.3639631629214257,
      -6.458245423578348,-6.458245423578355,-0.3639631629214177,1.223365632584434,
      -1.237281132017873,1.048637951819915,-0.8152389231864428,-0.4900563063939873,
      -0.4900563063939785,-0.8152389231864419,1.808916611395365,1.327623829722838,
      1.808916611395357,-4.276280903149721,-4.276280903149711,13.89283735202937
  };
  double* table[4] = {f3,f4,f5,f6};
  int nb = (P-1)*(P-2)/2;
  int ni = (P+1)*(P+2)/2;
  c.allocate(ni*nb);
  for( int i = 0; i < nb; ++i)
    for( int j = 0; j < ni; ++j)
      c[i*ni+j] = table[P-3][i*ni+j];
}

static void getBezierTetTransform(int P, apf::NewArray<double> & c)
{
  assert(P == 4 && getBlendingOrder() == 0);
  double t4[35] = {
      -0.665492638178598,-0.665492638178598,-0.665492638178598,-0.665492638178598,
      0.697909481209196,0.496340368840329,0.697909481209197,0.697909481209196,
      0.496340368840329,0.697909481209196,0.697909481209196,0.49634036884033,
      0.697909481209196,0.697909481209196,0.496340368840329,0.697909481209196,
      0.697909481209196,0.496340368840329,0.697909481209196,0.697909481209196,
      0.496340368840329,0.697909481209196,-0.764902170896025,-0.764902170896025,
      -0.764902170896025,-0.764902170896025,-0.764902170896025,-0.764902170896025,
      -0.764902170896025,-0.764902170896025,-0.764902170896025,-0.764902170896025,
      -0.764902170896025,-0.764902170896025,10.6666666666667,
  };

  int nb = (P-1)*(P-2)*(P-3)/6;
  int ni = (P+1)*(P+2)*(P+3)/6;

  c.allocate(ni*nb);
  for( int i = 0; i < nb; ++i)
    for( int j = 0; j < ni; ++j)
      c[i*ni+j] = t4[i*ni+j];
}

static void getBlendedBezierTriangleTransform(int P, int blend,
    apf::NewArray<double> & c)
{
  double f3_1[9] = {
      -0.9166666666666666,-0.9166666666666666,-0.9166666666666666,0.625,
      0.625,0.625,0.625,0.625,0.625};
  double f4_1[36] = {
      -0.9652522174935585,-0.4749278524285263,-0.474927852428525,0.9544734242852975,
      0.2114582311122364,-0.3558323000932463,0.3162670836759962,0.6623750443900446,
      0.3162670836759965,-0.3558323000932466,0.2114582311122365,0.9544734242852969,
      -0.4749278524285266,-0.9652522174935582,-0.4749278524285267,-0.3558323000932466,
      0.2114582311122362,0.9544734242852964,0.9544734242852971,0.2114582311122362,
      -0.3558323000932463,0.3162670836759968,0.6623750443900446,0.3162670836759962,
      -0.4749278524285246,-0.4749278524285249,-0.9652522174935577,0.316267083675996,
      0.6623750443900441,0.3162670836759964,-0.3558323000932463,0.2114582311122363,
      0.9544734242852962,0.9544734242852965,0.211458231112236,-0.3558323000932463};
  double f5_1[90] = {
      -0.9896190564309203,-0.738808420216384,-0.7388084202163848,0.9658440657862472,
      0.04171314109496013,-0.1057339468933634,0.1969998484341598,0.1092166220563664,
      0.5255782179534745,0.5255782179534748,0.1092166220563671,0.1969998484341602,
      -0.105733946893363,0.04171314109495989,0.9658440657862463,-0.6705499632464713,
      -0.6705499632464708,0.5348746466380395,-0.5548347454926342,
      0.8556129752022299,0.8556129752022309,-0.5548347454926315,0.9039582836532245,
      0.4713275419819926,-0.4080056774571432,-0.364945737960221,-0.3649457379602211,
      -0.4080056774571429,0.4713275419819928,0.9039582836532244,-0.7388084202163845,
      -0.9896190564309206,-0.7388084202163844,0.1969998484341603,
      -0.1057339468933628,0.04171314109496014,0.9658440657862463,0.9658440657862467,
      0.04171314109495996,-0.1057339468933635,0.19699984843416,0.1092166220563669,
      0.5255782179534746,0.5255782179534746,0.109216622056367,-0.6705499632464688,
      0.5348746466380385,-0.6705499632464688,0.9039582836532237,
      0.4713275419819923,-0.4080056774571426,-0.3649457379602208,-0.3649457379602204,
      -0.4080056774571427,0.4713275419819917,0.9039582836532231,-0.5548347454926325,
      0.8556129752022298,0.8556129752022297,-0.5548347454926322,0.53487464663804,
      -0.6705499632464689,-0.6705499632464714,-0.3649457379602198,
      -0.4080056774571434,0.4713275419819908,0.9039582836532236,-0.5548347454926339,
      0.8556129752022303,0.8556129752022317,-0.5548347454926318,0.9039582836532237,
      0.4713275419819922,-0.4080056774571433,-0.3649457379602221,-0.7388084202163852,
      -0.7388084202163846,-0.9896190564309205,0.1092166220563666,
      0.5255782179534747,0.525578217953475,0.1092166220563669,0.1969998484341604,
      -0.1057339468933632,0.04171314109495965,0.9658440657862466,0.9658440657862468,
      0.04171314109496022,-0.1057339468933629,0.1969998484341604};
  double f6_1[180] = {
      -0.999202672284534,-0.5486647945148209,-0.548664794514821,0.995670322541919,
      0.03327041692752502,-0.03539024197228978,0.05085992010436882,-0.1093743778579856,
      -0.005509830686536889,0.3244661409802137,0.5885475612397376,0.3244661409802137,
      -0.005509830686537575,-0.1093743778579854,0.05085992010436882,-0.03539024197228994,
      0.03327041692752734,0.9956703225419204,-0.7451537342541176,-0.8340203261624907,
      0.07153891407889877,-0.4247309020959472,
      1.139230846114376,0.2947155220859971,-0.5425640903991483,0.3327297042933895,
      0.8307940601175486,0.7497997736251635,-0.2767201127078383,-0.659027551482725,
      -0.2321015132856476,0.320990500035157,0.2369417831281099,-0.2385262583092118,
      0.05390677566230472,0.9221966095561851,-0.8340203261624884,-0.7451537342541191,
      0.07153891407889752,0.332729704293389,
      -0.5425640903991491,0.2947155220859965,1.139230846114373,-0.4247309020959487,
      0.9221966095561898,0.05390677566230847,-0.2385262583092137,0.236941783128109,
      0.3209905000351572,-0.2321015132856468,-0.6590275514827235,-0.2767201127078409,
      0.749799773625164,0.8307940601175507,-0.5486647945148213,-0.9992026722845332,
      -0.5486647945148219,-0.1093743778579852,
      0.05085992010436929,-0.03539024197229033,0.03327041692752693,0.9956703225419209,
      0.9956703225419181,0.0332704169275252,-0.03539024197228969,0.05085992010436889,
      -0.1093743778579852,-0.005509830686537071,0.3244661409802135,0.5885475612397369,
      0.3244661409802133,-0.005509830686537369,-0.745153734254119,0.07153891407889693,
      -0.8340203261624911,0.9221966095561902,
      0.05390677566230899,-0.2385262583092128,0.2369417831281095,0.3209905000351567,
      -0.232101513285646,-0.6590275514827236,-0.2767201127078408,0.749799773625165,
      0.8307940601175522,0.3327297042933883,-0.5425640903991487,0.2947155220859963,
      1.139230846114373,-0.4247309020959492,0.1666631711105592,0.1666631711105653,
      0.1666631711105588,-1.020597677079591,
      0.3105678162305723,1.586729883920795,0.3105678162305752,-1.020597677079589,
      -1.020597677079595,0.3105678162305735,1.5867298839208,0.3105678162305785,
      -1.020597677079589,-1.020597677079591,0.3105678162305727,1.586729883920799,
      0.3105678162305789,-1.020597677079589,0.07153891407889752,-0.7451537342541207,
      -0.8340203261624857,0.3209905000351556,
      0.2369417831281083,-0.2385262583092101,0.053906775662307,0.9221966095561845,
      -0.4247309020959432,1.139230846114375,0.2947155220859968,-0.5425640903991487,
      0.3327297042933872,0.8307940601175496,0.7497997736251643,-0.2767201127078364,
      -0.6590275514827223,-0.232101513285647,-0.8340203261624878,0.07153891407889736,
      -0.7451537342541146,0.8307940601175472,
      0.7497997736251634,-0.2767201127078376,-0.6590275514827242,-0.2321015132856471,
      0.3209905000351583,0.2369417831281098,-0.2385262583092123,0.05390677566230425,
      0.9221966095561831,-0.4247309020959464,1.139230846114375,0.2947155220859961,
      -0.5425640903991491,0.3327297042933899,0.07153891407889712,-0.8340203261624897,
      -0.7451537342541225,-0.2321015132856457,
      -0.6590275514827209,-0.2767201127078385,0.7497997736251644,0.8307940601175504,
      0.3327297042933867,-0.5425640903991487,0.2947155220859971,1.139230846114373,
      -0.4247309020959465,0.9221966095561859,0.05390677566230901,-0.2385262583092126,
      0.2369417831281071,0.3209905000351557,-0.5486647945148214,-0.5486647945148212,
      -0.999202672284534,-0.005509830686536803,
      0.3244661409802132,0.5885475612397368,0.3244661409802139,-0.005509830686537201,
      -0.1093743778579852,0.05085992010436879,-0.03539024197229011,0.03327041692752735,
      0.9956703225419209,0.9956703225419202,0.03327041692752517,-0.03539024197228983,
      0.05085992010436948,-0.1093743778579855};
  double f3_2[9] = {
      -0.1666666666666667,-0.1666666666666667,-0.1666666666666667,
      0.25,0.25,0.25,0.25,0.25,0.25};
  double f4_2[36] = {
      -0.4869153443383633,0.0003941257550036311,0.0003941257550037788,0.593245256335027,
      0.1810102640100068,-0.1679256083483691,0.04693453767005338,0.1795981934949205,
      0.04693453767005338,-0.167925608348369,0.181010264010007,0.5932452563350273,
      0.0003941257550036313,-0.4869153443383633,0.0003941257550035421,-0.1679256083483691,
      0.1810102640100067,0.5932452563350268,0.5932452563350268,0.1810102640100067,
      -0.1679256083483691,0.04693453767005345,0.1795981934949204,0.04693453767005323,
      0.0003941257550036312,0.0003941257550038111,-0.486915344338363,0.04693453767005334,
      0.1795981934949204,0.04693453767005355,-0.1679256083483689,0.1810102640100068,
      0.593245256335027,0.5932452563350267,0.1810102640100066,-0.1679256083483692};
  double f5_2[90] = {
      -0.590287253140672,-0.05944006234084315,-0.05944006234084336,0.7324606222111002,
      0.05375670091962843,-0.1045975703221206,0.093800681892637,-0.01251023737019824,
      0.09167349158013202,0.09167349158013219,-0.01251023737019812,0.09380068189263722,
      -0.1045975703221203,0.05375670091962814,0.7324606222110998,-0.2022233819483696,
      -0.2022233819483693,0.1273085389974248,-0.3538617028423738,
      0.5692411352282479,0.5692411352282494,-0.3538617028423717,0.3618907339160508,
      0.2717372261280958,-0.1181165862556567,-0.09232169372470868,-0.09232169372470866,
      -0.1181165862556565,0.271737226128096,0.3618907339160501,-0.05944006234084306,
      -0.5902872531406722,-0.05944006234084313,0.09380068189263777,
      -0.1045975703221202,0.05375670091962828,0.7324606222110995,0.7324606222111002,
      0.05375670091962804,-0.1045975703221206,0.09380068189263724,-0.01251023737019808,
      0.0916734915801322,0.09167349158013183,-0.01251023737019751,-0.2022233819483686,
      0.1273085389974246,-0.2022233819483684,0.3618907339160508,
      0.2717372261280957,-0.1181165862556563,-0.0923216937247089,-0.09232169372470855,
      -0.1181165862556566,0.2717372261280955,0.3618907339160505,-0.3538617028423727,
      0.5692411352282487,0.5692411352282483,-0.3538617028423721,0.1273085389974246,
      -0.2022233819483685,-0.2022233819483696,-0.09232169372470923,
      -0.1181165862556561,0.2717372261280946,0.3618907339160509,-0.3538617028423732,
      0.5692411352282493,0.5692411352282497,-0.3538617028423727,0.3618907339160504,
      0.2717372261280954,-0.1181165862556559,-0.09232169372471029,-0.05944006234084328,
      -0.05944006234084331,-0.5902872531406722,-0.01251023737019801,
      0.09167349158013198,0.09167349158013233,-0.01251023737019818,0.09380068189263735,
      -0.1045975703221204,0.05375670091962784,0.7324606222111,0.7324606222111001,
      0.05375670091962843,-0.1045975703221204,0.09380068189263756};
  double f6_2[180] = {
      -0.6655728551217123,0.001451713411202747,0.001451713411202747,0.7923598915695561,
      0.01961075722455755,-0.03515177437178062,0.05598827748523288,-0.05004515323601956,
      -0.02273165057473245,0.02808104160185218,0.08644664890196471,0.02808104160185209,
      -0.02273165057473254,-0.05004515323601938,0.05598827748523314,-0.03515177437177898,
      0.01961075722455968,0.792359891569557,-0.3331503353209324,-0.2923076412630835,
      -0.0006435940104900974,-0.3576597173300187,
      0.8626594953511336,0.3024811253809449,-0.3585035122902753,0.2065441187469552,
      0.2130585145058838,0.293740375314208,-0.01431663885122083,-0.1499858466660513,
      -0.01729581041074021,0.09522877581028433,0.04927576983999792,-0.174778516237689,
      0.1081298070707562,0.5675236303603435,-0.2923076412630831,-0.333150335320933,
      -0.0006435940104903373,0.2065441187469537,
      -0.3585035122902769,0.3024811253809424,0.8626594953511296,-0.3576597173300213,
      0.5675236303603441,0.1081298070707588,-0.1747785162376885,0.04927576983999696,
      0.0952287758102842,-0.0172958104107397,-0.1499858466660504,-0.01431663885121944,
      0.2937403753142084,0.213058514505884,0.001451713411202591,-0.6655728551217115,
      0.001451713411202546,-0.05004515323601923,
      0.05598827748523329,-0.03515177437177925,0.01961075722455941,0.7923598915695571,
      0.7923598915695554,0.0196107572245576,-0.03515177437178041,0.055988277485233,
      -0.0500451532360191,-0.02273165057473253,0.02808104160185206,0.08644664890196443,
      0.02808104160185194,-0.02273165057473244,-0.3331503353209323,-0.000643594010490309,
      -0.2923076412630836,0.5675236303603438,
      0.1081298070707589,-0.1747785162376875,0.04927576983999721,0.09522877581028341,
      -0.01729581041073984,-0.1499858466660505,-0.01431663885121996,0.2937403753142087,
      0.2130585145058844,0.2065441187469537,-0.358503512290277,0.3024811253809426,
      0.8626594953511302,-0.3576597173300211,0.2570807924479978,0.2570807924480007,
      0.2570807924479996,-0.440627370374738,
      0.1056459154829377,0.7462154506689271,0.1056459154829407,-0.4406273703747371,
      -0.44062737037474,0.1056459154829386,0.7462154506689319,0.1056459154829424,
      -0.4406273703747383,-0.44062737037474,0.1056459154829379,0.7462154506689285,
      0.1056459154829424,-0.4406273703747386,-0.000643594010489832,-0.3331503353209349,
      -0.2923076412630825,0.09522877581028356,
      0.04927576983999716,-0.1747785162376877,0.1081298070707577,0.5675236303603433,
      -0.3576597173300161,0.8626594953511333,0.3024811253809445,-0.3585035122902759,
      0.2065441187469525,0.2130585145058841,0.2937403753142084,-0.01431663885122003,
      -0.1499858466660503,-0.01729581041074012,-0.2923076412630833,-0.0006435940104905721,
      -0.3331503353209324,0.2130585145058835,
      0.2937403753142081,-0.01431663885122018,-0.1499858466660509,-0.01729581041073994,
      0.09522877581028462,0.04927576983999771,-0.1747785162376896,0.1081298070707559,
      0.5675236303603424,-0.3576597173300179,0.8626594953511336,0.3024811253809441,
      -0.358503512290276,0.2065441187469555,-0.0006435940104899266,-0.2923076412630822,
      -0.3331503353209341,-0.01729581041074012,
      -0.1499858466660494,-0.01431663885121897,0.2937403753142083,0.2130585145058831,
      0.2065441187469521,-0.3585035122902767,0.3024811253809418,0.8626594953511302,
      -0.3576597173300175,0.5675236303603427,0.1081298070707584,-0.1747785162376866,
      0.04927576983999619,0.09522877581028362,0.001451713411202693,0.001451713411202515,
      -0.6655728551217119,-0.02273165057473234,
      0.02808104160185188,0.08644664890196443,0.02808104160185214,-0.02273165057473232,
      -0.05004515323601907,0.05598827748523308,-0.03515177437177872,0.01961075722455975,
      0.792359891569556,0.7923598915695564,0.01961075722455784,-0.03515177437178078,
      0.05598827748523321,-0.05004515323601966};
  double* table1[4] = {f3_1,f4_1,f5_1,f6_1};
  double* table2[4] = {f3_2,f4_2,f5_2,f6_2};
  double** table[2] = {table1,table2};
  int nb = (P-1)*(P-2)/2;
  int ni = (P+1)*(P+2)/2-nb;
  c.allocate(ni*nb);
  for( int i = 0; i < nb; ++i)
    for( int j = 0; j < ni; ++j)
      c[i*ni+j] = table[blend-1][P-3][i*ni+j];
}

static void getBlendedBezierTetTransform(int P, int blend,
    apf::NewArray<double> & c)
{
  assert(P == 4 && getBlendingOrder() == 0);
  double t4_1[34] = {
      1.921296296296296,1.921296296296296,1.921296296296296,1.921296296296296,
      -0.7098765432098767,-1.064814814814814,-0.7098765432098767,-0.7098765432098767,
      -1.064814814814814,-0.7098765432098767,-0.7098765432098767,-1.064814814814814,
      -0.7098765432098767,-0.7098765432098767,-1.064814814814814,-0.7098765432098767,
      -0.7098765432098767,-1.064814814814814,-0.7098765432098767,-0.7098765432098767,
      -1.064814814814814,-0.7098765432098767,0.342592592592593,0.342592592592593,
      0.342592592592593,0.342592592592593,0.342592592592593,0.342592592592593,
      0.342592592592593,0.342592592592593,0.342592592592593,0.342592592592593,
      0.342592592592593,0.342592592592593};
  double t4_2[34] = {
      0.3472222222222222,0.3472222222222222,0.3472222222222222,0.3472222222222222,
      -0.2407407407407403,-0.3611111111111111,-0.2407407407407404,-0.2407407407407404,
      -0.3611111111111111,-0.2407407407407404,-0.2407407407407404,-0.3611111111111111,
      -0.2407407407407404,-0.2407407407407404,-0.3611111111111111,-0.2407407407407404,
      -0.2407407407407404,-0.3611111111111111,-0.2407407407407404,-0.2407407407407404,
      -0.3611111111111111,-0.2407407407407404,0.388888888888888,0.3888888888888888,
      0.3888888888888888,0.3888888888888888,0.3888888888888888,0.3888888888888888,
      0.3888888888888888,0.3888888888888888,0.3888888888888888,0.3888888888888888,
      0.3888888888888888,0.3888888888888888};
  double* t4[2] = {t4_1,t4_2};
  int nb = (P-1)*(P-2)*(P-3)/6;
  int ni = (P+1)*(P+2)*(P+3)/6 - nb;

  c.allocate(ni*nb);
  for( int i = 0; i < nb; ++i)
    for( int j = 0; j < ni; ++j)
      c[i*ni+j] = t4[blend-1][i*ni+j];
}

void getTransformationCoefficients(int P, int type,
    apf::NewArray<double>& c){

  if(type == apf::Mesh::EDGE)
    getBezierEdgeTransform(P,c);
  else if(type == apf::Mesh::TRIANGLE)
    getBezierTriangleTransform(P,c);
  else if(type == apf::Mesh::TET)
    getBezierTetTransform(P,c);
}

void getBlendedTransformationCoefficients(int P, int blend, int type,
    apf::NewArray<double>& c){

  if(type == apf::Mesh::TRIANGLE)
    getBlendedBezierTriangleTransform(P,blend,c);
  else if(type == apf::Mesh::TET)
    getBlendedBezierTetTransform(P,blend,c);
}

static void getGregoryTriangleTransform(int P, apf::NewArray<double> & c)
{
  apf::NewArray<double> d;
  getBezierTriangleTransform(P,d);

  int nbBezier = (P-1)*(P-2)/2;
  int niBezier = (P+1)*(P+2)/2;

  int nb = 6;
  int ni = 3*P;
  c.allocate(ni*nb);

  int map[3] = {1,2,0};
  // copy the bezier point locations
  for(int i = 0; i < nbBezier; ++i){
    for(int j = 0; j < niBezier; ++j)
      c[i*ni+j] = d[i*niBezier+j];
    for(int j = niBezier; j < ni; ++j)
      c[i*ni+j] = 0.;
  }
  if(P == 3){
    for(int i = nbBezier; i < nb; ++i){
      for(int j = 0; j < niBezier; ++j)
        c[i*ni+j] = d[j];
      for(int j = niBezier; j < ni; ++j)
        c[i*ni+j] = 0.;
    }
  }
  if(P == 4){
    for(int i = nbBezier; i < nb; ++i){
      for(int j = 0; j < niBezier; ++j)
        c[i*ni+j] = d[map[i-nbBezier]*niBezier+j];
      for(int j = niBezier; j < ni; ++j)
        c[i*ni+j] = 0.;
    }
  }
}

static void getGregoryTetTransform(int P, apf::NewArray<double> & c)
{
  assert(P == 4 && getBlendingOrder() == 0);
  double t4[47] = {
      -0.665492638178598,-0.665492638178598,-0.665492638178598,-0.665492638178598,
      0.697909481209196,0.496340368840329,0.697909481209197,0.697909481209196,
      0.496340368840329,0.697909481209196,0.697909481209196,0.49634036884033,
      0.697909481209196,0.697909481209196,0.496340368840329,0.697909481209196,
      0.697909481209196,0.496340368840329,0.697909481209196,0.697909481209196,
      0.496340368840329,0.697909481209196,
      -0.764902170896025,-0.764902170896025,-0.764902170896025,-0.764902170896025,
      -0.764902170896025,-0.764902170896025,
      -0.764902170896025,-0.764902170896025,-0.764902170896025,-0.764902170896025,
      -0.764902170896025,-0.764902170896025,
      -0.764902170896025,-0.764902170896025,-0.764902170896025,-0.764902170896025,
      -0.764902170896025,-0.764902170896025,
      -0.764902170896025,-0.764902170896025,-0.764902170896025,-0.764902170896025,
      -0.764902170896025,-0.764902170896025,
      10.6666666666667,
  };

  c.allocate(47);
  for (int j = 0; j < 47; ++j)
    c[j] = t4[j];
}

static void getBlendedGregoryTriangleTransform(int P, int blend,
    apf::NewArray<double> & c)
{
  apf::NewArray<double> d;
  getBlendedBezierTriangleTransform(P,blend,d);

  int nbBezier = (P-1)*(P-2)/2;
  int niBezier = (P+1)*(P+2)/2-nbBezier;

  int nb = 6;
  int ni = 6+3*P;
  c.allocate(ni*nb);

  int map[3] = {1,2,0};
  // copy the bezier point locations
  for(int i = 0; i < nbBezier; ++i){
    for(int j = 0; j < niBezier; ++j)
      c[i*ni+j] = d[i*niBezier+j];
    for(int j = niBezier; j < ni; ++j)
      c[i*ni+j] = 0.;
  }
  if(P == 3){
    for(int i = nbBezier; i < nb; ++i){
      for(int j = 0; j < niBezier; ++j)
        c[i*ni+j] = d[j];
      for(int j = niBezier; j < ni; ++j)
        c[i*ni+j] = 0.;
    }
  }
  if(P == 4){
    for(int i = nbBezier; i < nb; ++i){
      for(int j = 0; j < niBezier; ++j)
        c[i*ni+j] = d[map[i-nbBezier]*niBezier+j];
      for(int j = niBezier; j < ni; ++j)
        c[i*ni+j] = 0.;
    }
  }
}

static void getBlendedGregoryTetTransform(int P, int blend,
    apf::NewArray<double> & c)
{
  assert(P == 4 && getBlendingOrder() == 0);
  double t4_1[46] = {
      1.921296296296296,1.921296296296296,1.921296296296296,1.921296296296296,
      -0.7098765432098767,-1.064814814814814,-0.7098765432098767,-0.7098765432098767,
      -1.064814814814814,-0.7098765432098767,-0.7098765432098767,-1.064814814814814,
      -0.7098765432098767,-0.7098765432098767,-1.064814814814814,-0.7098765432098767,
      -0.7098765432098767,-1.064814814814814,-0.7098765432098767,-0.7098765432098767,
      -1.064814814814814,-0.7098765432098767,
      0.342592592592593,0.342592592592593,0.342592592592593,
      0.342592592592593,0.342592592592593,0.342592592592593,
      0.342592592592593,0.342592592592593,0.342592592592593,
      0.342592592592593,0.342592592592593,0.342592592592593,
      0.342592592592593,0.342592592592593,0.342592592592593,
      0.342592592592593,0.342592592592593,0.342592592592593,
      0.342592592592593,0.342592592592593,0.342592592592593,
      0.342592592592593,0.342592592592593,0.342592592592593};
  double t4_2[46] = {
      0.3472222222222222,0.3472222222222222,0.3472222222222222,0.3472222222222222,
      -0.2407407407407403,-0.3611111111111111,-0.2407407407407404,-0.2407407407407404,
      -0.3611111111111111,-0.2407407407407404,-0.2407407407407404,-0.3611111111111111,
      -0.2407407407407404,-0.2407407407407404,-0.3611111111111111,-0.2407407407407404,
      -0.2407407407407404,-0.3611111111111111,-0.2407407407407404,-0.2407407407407404,
      -0.3611111111111111,-0.2407407407407404,
      0.388888888888888,0.3888888888888888,0.3888888888888888,0.3888888888888888,
      0.3888888888888888,0.3888888888888888,
      0.3888888888888888,0.3888888888888888,0.3888888888888888,0.3888888888888888,
      0.3888888888888888,0.3888888888888888,
      0.388888888888888,0.3888888888888888,0.3888888888888888,0.3888888888888888,
      0.3888888888888888,0.3888888888888888,
      0.3888888888888888,0.3888888888888888,0.3888888888888888,0.3888888888888888,
      0.3888888888888888,0.3888888888888888};
  double* t4[2] = {t4_1,t4_2};
  c.allocate(46);
  for (int j = 0; j < 46; ++j)
    c[j] = t4[blend-1][j];
}

void getGregoryTransformationCoefficients(int P, int type,
    apf::NewArray<double>& c){
  assert(P == 3 || P == 4);
  if(type == apf::Mesh::EDGE)
    getBezierEdgeTransform(P,c);
  else if(type == apf::Mesh::TRIANGLE)
    getGregoryTriangleTransform(P,c);
  else if(type == apf::Mesh::TET)
    getGregoryTetTransform(P,c);
}

void getGregoryBlendedTransformationCoefficients(int P, int blend, int type,
    apf::NewArray<double>& c){

  if(type == apf::Mesh::TRIANGLE)
    getBlendedGregoryTriangleTransform(P,blend,c);
  else if(type == apf::Mesh::TET)
    getBlendedGregoryTetTransform(P,blend,c);
}

void getTransformationMatrix(apf::Mesh2* m, apf::MeshEntity* e,
    apf::DynamicMatrix& A)
{

  apf::Vector3 const edge_vert_xi[2] = {
      apf::Vector3(-1,0,0),
      apf::Vector3(1,0,0),
  };
  apf::Vector3 const tri_vert_xi[3] = {
      apf::Vector3(0,0,0),
      apf::Vector3(1,0,0),
      apf::Vector3(0,1,0),
  };
  apf::Vector3 const tet_vert_xi[4] = {
      apf::Vector3(0,0,0),
      apf::Vector3(1,0,0),
      apf::Vector3(0,1,0),
      apf::Vector3(0,0,1),
  };
  apf::Vector3 const* const elem_vert_xi[apf::Mesh::TYPES] = {
      0, /* vertex */
      edge_vert_xi,
      tri_vert_xi,
      0, /* quad */
      tet_vert_xi,
      0, /* hex */
      0, /* prism */
      0  /* pyramid */
  };

  int type = m->getType(e);
  apf::FieldShape* fs = m->getShape();
  apf::EntityShape* es = fs->getEntityShape(type);
  int n = es->countNodes();
  int typeDim = apf::Mesh::typeDimension[type];

  apf::Vector3 xi, exi;
  int evi = 0;
  apf::NewArray<double> values;

  A.setSize(n,n);
  A.zero();

  int boundaryTypes[4] = {apf::Mesh::VERTEX,apf::Mesh::EDGE,
      apf::Mesh::TRIANGLE,apf::Mesh::TET};

  int row = 0;
  for(int d = 0; d <= typeDim; ++d){
    int nDown = apf::Mesh::adjacentCount[type][d];
    for(int j = 0; j < nDown; ++j){
      int bt = boundaryTypes[d];
      apf::EntityShape* shape = apf::getLagrange(1)->getEntityShape(bt);

      for(int x = 0; x < fs->countNodesOn(bt); ++x){
        fs->getNodeXi(bt,x,xi);
        apf::NewArray<double> shape_vals;
        shape->getValues(0, 0, xi, shape_vals);

        if(d < typeDim){
          exi.zero();
          evi = j;
          for (int i = 0; i < apf::Mesh::adjacentCount[bt][0]; ++i) {
            if(bt == apf::Mesh::EDGE && type == apf::Mesh::TRIANGLE)
              evi = apf::tri_edge_verts[j][i];
            if(bt == apf::Mesh::EDGE && type == apf::Mesh::TET)
              evi = apf::tet_edge_verts[j][i];
            if(bt == apf::Mesh::TRIANGLE && type == apf::Mesh::TET)
              evi = apf::tet_tri_verts[j][i];
            assert(evi >= 0);
            exi += elem_vert_xi[type][evi] * shape_vals[i];
          }
        } else {
          exi = xi;
        }
        es->getValues(m,e,exi,values);
        for(int i = 0; i < n; ++i){
          A(row,i) = values[i];
        }
        ++row;
      }
    }
  }
}

} // namespace crv
