#include <iostream>
#include <complex>
#include <blitz/array.h>
#include <blitz/tinyvec-et.h>
#include <vector>
#include <algorithm>

using namespace blitz;

#include "EMConstants.h"
#include "GK_triangle.h"
#include "triangle_int.h"
#include "mesh.h"
#include "V_E_V_H.h"

void G_EJ_G_HJ (blitz::Array<std::complex<double>, 2>& G_EJ,
                blitz::Array<std::complex<double>, 2>& G_HJ,
                const blitz::TinyVector<double, 3>& r_dip,
                const blitz::TinyVector<double, 3>& r_obs,
                const std::complex<double>& eps,
                const std::complex<double>& mu,
                const std::complex<double>& k)
/**
 * This function computes the homogeneous space Green's functions
 * due to an elementary electric current element.
 *
 * By reciprocity, we have that G_EM = -G_HJ and G_HM = eps/mu * G_EJ.
 */
{
  double R = sqrt(dot(r_obs-r_dip, r_obs-r_dip));
  std::complex<double> term_1 = 1.0 + 1.0/(I*k*R);
  std::complex<double> term_2 = 1.0/R * term_1 + I*k/2.0 * (term_1 - 1.0/pow2(k*R));
  std::complex<double> exp_ikR = exp(-I*k*R), exp_ikR_R = exp_ikR/R;
  double x_xp = r_obs(0)-r_dip(0), y_yp = r_obs(1)-r_dip(1), z_zp = r_obs(2)-r_dip(2);
  G_EJ (0, 1) = term_2*y_yp/R*x_xp/R; 
  G_EJ (0, 2) = term_2*z_zp/R*x_xp/R; 
  G_EJ (1, 2) = term_2*z_zp/R*y_yp/R;
  G_EJ (1, 0) = G_EJ (0, 1);
  G_EJ (2, 0) = G_EJ (0, 2);
  G_EJ (2, 1) = G_EJ (1, 2);
  G_EJ (0, 0) = pow2(x_xp/R)*term_1/R-(1.0-pow2(x_xp/R))*I*k/2.0*(term_1 - 1.0/pow2(k*R));
  G_EJ (1, 1) = pow2(y_yp/R)*term_1/R-(1.0-pow2(y_yp/R))*I*k/2.0*(term_1 - 1.0/pow2(k*R));
  G_EJ (2, 2) = pow2(z_zp/R)*term_1/R-(1.0-pow2(z_zp/R))*I*k/2.0*(term_1 - 1.0/pow2(k*R));
  G_EJ *= sqrt(mu/eps)/(2.0*M_PI) * exp_ikR_R;

  std::complex<double> G_i = exp_ikR/(4.0*M_PI) * (1.0+I*k*R)/(R*R*R);
  G_HJ (0, 1) = (z_zp) * G_i;
  G_HJ (1, 0) = -G_HJ (0, 1);
  G_HJ (2, 0) = (y_yp) * G_i;
  G_HJ (2, 1) = -1.0*(x_xp) * G_i;
  G_HJ (0, 2) = -G_HJ (2, 0);
  G_HJ (1, 2) = -G_HJ (2, 1);
  for (int i=0 ; i<3 ; i++) G_HJ(i, i) = 0.0;
}

void V_EJ_HJ_dipole (blitz::Array<std::complex<double>, 1> V_tE_J,
                     blitz::Array<std::complex<double>, 1> V_nE_J,
                     blitz::Array<std::complex<double>, 1> V_tH_J,
                     blitz::Array<std::complex<double>, 1> V_nH_J,
                     const blitz::Array<std::complex<double>, 1>& J_dip,
                     const blitz::Array<double, 1>& r_dip,
                     const blitz::Array<int, 1>& numbers_RWG_test,
                     const blitz::Array<int, 1>& RWGNumber_CFIE_OK,
                     const blitz::Array<int, 2>& RWGNumber_signedTriangles,
                     const blitz::Array<double, 2>& RWGNumber_vertexesCoord,
                     const blitz::Array<double, 2>& RWGNumber_oppVertexesCoord,
                     const double w,
                     const std::complex<double>& eps_r,
                     const std::complex<double>& mu_r)
/**
 * This function computes the 4 excitation vectors of the MoM due to an
 * elementary electric current element J_dip located at r_dip.
 *
 * By reciprocity, we have that V_EM = -V_HJ and V_HM = eps/mu * V_EJ.
 */
{
  // def of k, mu_i, eps_i
  blitz::Range all = blitz::Range::all();
  int N_RWG_test = numbers_RWG_test.size();
  std::complex<double> mu = mu_0 * mu_r, eps = eps_0 * eps_r, k = w * sqrt(eps*mu);

  V_tE_J = 0.0; V_tH_J = 0.0;
  V_nE_J = 0.0; V_nH_J = 0.0;
  std::vector<RWG> test_RWGs;
  test_RWGs.reserve(N_RWG_test);
  for (int i=0 ; i<N_RWG_test ; ++i) {
    const int RWGnumber = numbers_RWG_test(i);
    blitz::TinyVector<int, 2> triangle_numbers, triangle_signs;
    triangle_numbers(0) = abs(RWGNumber_signedTriangles(RWGnumber, 0));
    triangle_numbers(1) = abs(RWGNumber_signedTriangles(RWGnumber, 1));
    triangle_signs(0) = 1;
    triangle_signs(1) = -1;
    // the nodes of the RWG
    blitz::Array<double, 1> r0(3), r1(3), r2(3), r3(3);
    r0 = RWGNumber_oppVertexesCoord(RWGnumber, blitz::Range(0,2));
    r3 = RWGNumber_oppVertexesCoord(RWGnumber, blitz::Range(3,5));
    r1 = RWGNumber_vertexesCoord(RWGnumber, blitz::Range(0,2));
    r2 = RWGNumber_vertexesCoord(RWGnumber, blitz::Range(3,5));
    test_RWGs.push_back(RWG(RWGnumber, triangle_numbers, triangle_signs, r0, r1, r2, r3));
  }
  // triangles
  std::vector< Dictionary<int, int> > testTriangleToRWG;
  testTriangleToRWG.reserve(N_RWG_test*2);
  for (int i=0 ; i<test_RWGs.size() ; ++i) {
    testTriangleToRWG.push_back(Dictionary<int, int>(test_RWGs[i].triangleNumbers(0), test_RWGs[i].number));
    testTriangleToRWG.push_back(Dictionary<int, int>(test_RWGs[i].triangleNumbers(1), test_RWGs[i].number));
  }
  sort(testTriangleToRWG.begin(), testTriangleToRWG.end());
  std::vector<Triangle> triangles_test;
  constructVectorTriangles(triangles_test, test_RWGs, testTriangleToRWG);

  // geometrical entities
  blitz::TinyVector<double, 3> r0, r1, r2, r_obs;
  std::complex<double> ITo_r_dot_H_inc, ITo_r_dot_E_inc, ITo_n_hat_X_r_dot_H_inc, ITo_n_hat_X_r_dot_E_inc;
  blitz::TinyVector<std::complex<double>, 3> H_inc_i, ITo_H_inc, E_inc_i, ITo_E_inc;
  blitz::Array<std::complex<double>, 2> G_EJ (3, 3), G_HJ (3, 3);
  blitz::TinyVector<double, 3> rDip;
  blitz::TinyVector<std::complex<double>, 3> JDip;
  for (int i=0 ; i<3 ; ++i) rDip(i) = r_dip(i);
  for (int i=0 ; i<3 ; ++i) JDip(i) = J_dip(i);

  for (int r=0 ; r<triangles_test.size() ; ++r) { // loop on the observation triangles
      // the RWGs concerned by the test triangle
      std::vector<int> RWGsIndexes_test(triangles_test[r].RWGIndexes);
      std::vector<int> triangleTest_indexesInRWGs(triangles_test[r].indexesInRWGs);
      std::vector<double> triangleTest_signsInRWGs(triangles_test[r].signInRWG);
      ITo_E_inc = 0.0; // TinyVector<complex<double>, 3>
      ITo_H_inc = 0.0; // TinyVector<complex<double>, 3>
      ITo_r_dot_E_inc = 0.0; // complex<double>
      ITo_r_dot_H_inc = 0.0; // complex<double>
      ITo_n_hat_X_r_dot_E_inc = 0.0; // complex<double>
      ITo_n_hat_X_r_dot_H_inc = 0.0; // complex<double>

      r0 = triangles_test[r].r_nodes(0);
      r1 = triangles_test[r].r_nodes(1);
      r2 = triangles_test[r].r_nodes(2);

      // weights and abscissas for triangle integration
      double R_os = sqrt (dot (triangles_test[r].r_grav - rDip, triangles_test[r].r_grav - rDip));
      bool IS_NEAR = (R_os - 1.5 * triangles_test[r].R_max <= 0.0);
      int N_points = 6;
      if (IS_NEAR) N_points = 9;
      double sum_weigths;
      const double *xi, *eta, *weigths;
      IT_points (xi, eta, weigths, sum_weigths, N_points);
      // triangle integration
      for (int j=0 ; j<N_points ; ++j) {
        r_obs = r0 * xi[j] + r1 * eta[j] + r2 * (1-xi[j]-eta[j]);
        blitz::TinyVector<double, 3> n_hat_X_r(cross(triangles_test[r].n_hat, r_obs));
        G_EJ_G_HJ (G_EJ, G_HJ, rDip, r_obs, eps, mu, k);

        // computation of ITo_E_inc due to a dipole located at r_dip
        for (int m=0 ; m<3 ; m++) E_inc_i (m) = (G_EJ (m, 0) * JDip (0) + G_EJ (m, 1) * JDip (1) + G_EJ (m, 2) * JDip (2)) * weigths[j];
        ITo_E_inc += E_inc_i;
        ITo_r_dot_E_inc += sum(r_obs * E_inc_i);
        ITo_n_hat_X_r_dot_E_inc += sum(n_hat_X_r * E_inc_i);

        // computation of ITo_H_inc due to a dipole located at r_dip
        for (int m=0 ; m<3 ; m++) H_inc_i (m) = (G_HJ (m, 0) * JDip (0) + G_HJ (m, 1) * JDip (1) + G_HJ (m, 2) * JDip (2)) * weigths[j];
        ITo_H_inc += H_inc_i;
        ITo_r_dot_H_inc += dot(r_obs, H_inc_i);
        ITo_n_hat_X_r_dot_H_inc += dot(n_hat_X_r, H_inc_i);
      }
      const double norm_factor = triangles_test[r].A/sum_weigths;

      ITo_E_inc *= norm_factor;
      ITo_H_inc *= norm_factor;
      ITo_r_dot_E_inc *= norm_factor;
      ITo_r_dot_H_inc *= norm_factor;
      ITo_n_hat_X_r_dot_E_inc *= norm_factor;
      ITo_n_hat_X_r_dot_H_inc *= norm_factor;

      for (int p=0 ; p<RWGsIndexes_test.size() ; ++p) {
        const int index_p = RWGsIndexes_test[p];
        const int local_number_edge_p = test_RWGs[index_p].number;
        const double l_p = test_RWGs[index_p].length;
        const double sign_edge_p = triangleTest_signsInRWGs[p];
        const double C_rp = sign_edge_p * l_p * 0.5/triangles_test[r].A;
        blitz::TinyVector<double, 3> r_p;
        if (triangleTest_indexesInRWGs[p]==0) r_p = test_RWGs[index_p].vertexesCoord(0);
        else if (triangleTest_indexesInRWGs[p]==1) r_p = test_RWGs[index_p].vertexesCoord(3);
        const TinyVector<double, 3> n_hat_X_r_p(cross(triangles_test[r].n_hat, r_p));

        V_tE_J (local_number_edge_p) += -C_rp * (ITo_r_dot_E_inc - sum(r_p * ITo_E_inc)); // -<f_m ; E_inc> 
        V_tH_J (local_number_edge_p) += -C_rp * (ITo_r_dot_H_inc - sum(r_p * ITo_H_inc)); // -<f_m ; H_inc>
        V_nE_J (local_number_edge_p) += -C_rp * (ITo_n_hat_X_r_dot_E_inc - sum(n_hat_X_r_p * ITo_E_inc)); // -<n_hat x f_m ; E_inc> 
        V_nH_J (local_number_edge_p) += -C_rp * (ITo_n_hat_X_r_dot_H_inc - sum(n_hat_X_r_p * ITo_H_inc)); // -<n_hat x f_m ; H_inc> 
      }
  }
}

void V_CFIE_dipole (blitz::Array<std::complex<float>, 1> V_CFIE,
                    const blitz::Array<std::complex<float>, 1>& CFIE,
                    const blitz::Array<std::complex<double>, 1>& J_dip,
                    const blitz::Array<double, 1>& r_dip,
                    const blitz::Array<int, 1>& numbers_RWG_test,
                    const blitz::Array<int, 1>& RWGNumber_CFIE_OK,
                    const blitz::Array<double, 2>& RWGNumber_trianglesCoord,
                    const double w,
                    const std::complex<double>& eps_r,
                    const std::complex<double>& mu_r)
/**
 * This function computes the CFIE excitation vectors of the MoM due to an
 * elementary electric current element J_dip located at r_dip.
 *
 */
{
  // def of k, mu_i, eps_i
  blitz::Range all = blitz::Range::all();
  int N_RWG_test = numbers_RWG_test.size();
  std::complex<double> mu = mu_0 * mu_r, eps = eps_0 * eps_r, k = w * sqrt(eps*mu);
  const complex<double> tE = CFIE(0), nE = CFIE(1), tH = CFIE(2), nH = CFIE(3);

  // geometrical entities
  blitz::TinyVector<double, 3> r0, r1, r2, r_obs;
  std::complex<double> ITo_r_dot_H_inc, ITo_r_dot_E_inc, ITo_n_hat_X_r_dot_H_inc, ITo_n_hat_X_r_dot_E_inc;
  blitz::TinyVector<std::complex<double>, 3> H_inc_i, ITo_H_inc, E_inc_i, ITo_E_inc;
  blitz::Array<std::complex<double>, 2> G_EJ (3, 3), G_HJ (3, 3);
  blitz::TinyVector<double, 3> rDip;
  blitz::TinyVector<std::complex<double>, 3> JDip;
  for (int i=0 ; i<3 ; ++i) rDip(i) = r_dip(i);
  for (int i=0 ; i<3 ; ++i) JDip(i) = J_dip(i);

  V_CFIE = 0.0;

  for (int rwg=0 ; rwg<N_RWG_test ; ++rwg) { // loop on the RWGs
    for (int tr = 0 ; tr<2 ; ++tr) {
      double l_p;
      blitz::Array<double, 1> rt0(3), rt1(3), rt2(3), r_opp(3);
      if (tr==0) {
        rt0 = RWGNumber_trianglesCoord(rwg, blitz::Range(0,2));
        rt1 = RWGNumber_trianglesCoord(rwg, blitz::Range(3,5));
        rt2 = RWGNumber_trianglesCoord(rwg, blitz::Range(6,8));
        for (int i=0 ; i<3 ; i++) {
          r0(i) = rt0(i);
          r1(i) = rt1(i);
          r2(i) = rt2(i);
        }
        r_opp = rt0;
        l_p = sqrt(sum((rt1-rt2) * (rt1-rt2)));
      }
      else{
        rt0 = RWGNumber_trianglesCoord(rwg, blitz::Range(6,8));
        rt1 = RWGNumber_trianglesCoord(rwg, blitz::Range(3,5));
        rt2 = RWGNumber_trianglesCoord(rwg, blitz::Range(9,11));
        for (int i=0 ; i<3 ; i++) {
          r0(i) = rt0(i);
          r1(i) = rt1(i);
          r2(i) = rt2(i);
        }
        r_opp = rt2;
        l_p = sqrt(sum((rt0-rt1) * (rt0-rt1)));
      }
      Triangle triangle(r0, r1, r2, 0);
      ITo_E_inc = 0.0; // TinyVector<complex<double>, 3>
      ITo_H_inc = 0.0; // TinyVector<complex<double>, 3>
      ITo_r_dot_E_inc = 0.0; // complex<double>
      ITo_r_dot_H_inc = 0.0; // complex<double>
      ITo_n_hat_X_r_dot_E_inc = 0.0; // complex<double>
      ITo_n_hat_X_r_dot_H_inc = 0.0; // complex<double>

      r0 = triangle.r_nodes(0);
      r1 = triangle.r_nodes(1);
      r2 = triangle.r_nodes(2);
      // weights and abscissas for triangle integration
      double R_os = sqrt (dot (triangle.r_grav - rDip, triangle.r_grav - rDip));
      bool IS_NEAR = (R_os - 1.5 * triangle.R_max <= 0.0);
      int N_points = 6;
      if (IS_NEAR) N_points = 9;
      double sum_weigths;
      const double *xi, *eta, *weigths;
      IT_points (xi, eta, weigths, sum_weigths, N_points);
      // triangle integration
      for (int j=0 ; j<N_points ; ++j) {
        r_obs = r0 * xi[j] + r1 * eta[j] + r2 * (1-xi[j]-eta[j]);
        blitz::TinyVector<double, 3> n_hat_X_r(cross(triangle.n_hat, r_obs));
        G_EJ_G_HJ (G_EJ, G_HJ, rDip, r_obs, eps, mu, k);

        // computation of ITo_E_inc due to a dipole located at r_dip
        for (int m=0 ; m<3 ; m++) E_inc_i (m) = (G_EJ (m, 0) * JDip (0) + G_EJ (m, 1) * JDip (1) + G_EJ (m, 2) * JDip (2)) * weigths[j];
        ITo_E_inc += E_inc_i;
        ITo_r_dot_E_inc += sum(r_obs * E_inc_i);
        ITo_n_hat_X_r_dot_E_inc += sum(n_hat_X_r * E_inc_i);

        // computation of ITo_H_inc due to a dipole located at r_dip
        for (int m=0 ; m<3 ; m++) H_inc_i (m) = (G_HJ (m, 0) * JDip (0) + G_HJ (m, 1) * JDip (1) + G_HJ (m, 2) * JDip (2)) * weigths[j];
        ITo_H_inc += H_inc_i;
        ITo_r_dot_H_inc += dot(r_obs, H_inc_i);
        ITo_n_hat_X_r_dot_H_inc += dot(n_hat_X_r, H_inc_i);
      }
      const double norm_factor = triangle.A/sum_weigths;

      ITo_E_inc *= norm_factor;
      ITo_H_inc *= norm_factor;
      ITo_r_dot_E_inc *= norm_factor;
      ITo_r_dot_H_inc *= norm_factor;
      ITo_n_hat_X_r_dot_E_inc *= norm_factor;
      ITo_n_hat_X_r_dot_H_inc *= norm_factor;

      const int local_number_edge_p = numbers_RWG_test(rwg);
      const int sign_edge_p = (tr==0) ? 1 : -1;
      const double C_rp = sign_edge_p * l_p * 0.5/triangle.A;
      blitz::TinyVector<double, 3> r_p;
      for (int i=0; i<3 ; ++i) r_p(i) = r_opp(i);
      const TinyVector<double, 3> n_hat_X_r_p(cross(triangle.n_hat, r_p));

      complex<double> tmpResult(0.0, 0.0);
      tmpResult -= tE * C_rp * (ITo_r_dot_E_inc - sum(r_p * ITo_E_inc)); // -<f_m ; E_inc>
      if (RWGNumber_CFIE_OK(local_number_edge_p) == 1) {
        tmpResult -= nE * C_rp * (ITo_n_hat_X_r_dot_E_inc - sum(n_hat_X_r_p * ITo_E_inc)); // -<n_hat x f_m ; E_inc> 
        tmpResult -= tH * C_rp * (ITo_r_dot_H_inc - sum(r_p * ITo_H_inc)); // -<f_m ; H_inc>
        tmpResult -= nH * C_rp * (ITo_n_hat_X_r_dot_H_inc - sum(n_hat_X_r_p * ITo_H_inc)); // -<n_hat x f_m ; H_inc> 
      }
      V_CFIE(local_number_edge_p) += tmpResult;
    }
  }
}

void local_V_CFIE_dipole (blitz::Array<std::complex<float>, 1>& V_CFIE,
                           const blitz::Array<std::complex<double>, 1>& J_dip,
                           const blitz::Array<double, 1>& r_dip,
                           const LocalMesh & local_target_mesh,
                           const double w,
                           const std::complex<double>& eps_r,
                           const std::complex<double>& mu_r,
                           const blitz::Array<std::complex<float>, 1>& CFIE)
{
  // We now compute the excitation vectors
  V_CFIE.resize(local_target_mesh.N_local_RWG);
  V_CFIE_dipole (V_CFIE, CFIE, J_dip, r_dip, local_target_mesh.reallyLocalRWGNumbers, local_target_mesh.localRWGNumber_CFIE_OK, local_target_mesh.localRWGNumber_trianglesCoord, w, eps_r, mu_r);
}


/*void V_EJ_HJ_dipole_alternative (blitz::Array<std::complex<double>, 1> V_tE_J,
                                 blitz::Array<std::complex<double>, 1> V_nE_J,
                                 blitz::Array<std::complex<double>, 1> V_tH_J,
                                 blitz::Array<std::complex<double>, 1> V_nH_J,
                                 const blitz::Array<std::complex<double>, 1>& J_dip,
                                 const blitz::Array<double, 1>& r_dip,
                                 const blitz::Array<int, 1>& numbers_RWG_test,
                                 const blitz::Array<int, 1>& RWGNumber_CFIE_OK,
                                 const blitz::Array<int, 2>& RWGNumber_signedTriangles,
                                 const blitz::Array<double, 2>& RWGNumber_vertexesCoord,
                                 const blitz::Array<double, 2>& RWGNumber_oppVertexesCoord,
                                 const double w,
                                 const std::complex<double>& eps_r,
                                 const std::complex<double>& mu_r)
{
  // This alternative tries to get rid of the singularities associated with a dipole source
  // located near the surface of interest. This function can be used for computing the fields
  // near surface currents distributions by reciprocity.

  // def of k, mu_i, eps_i
  Range all = Range::all();
  int N_RWG_test = numbers_RWG_test.size(), EXTRACT_R, EXTRACT_1_R, N_points;
  complex<double> mu = mu_0 * mu_r, eps = eps_0 * eps_r, k = w * sqrt(eps*mu);

  // transformation of some input Arrays into TinyVectors...
  blitz::TinyVector<double, 3> rDip;
  blitz::TinyVector<std::complex<double>, 3> JDip;
  for (int i=0; i<3; i++) rDip(i) = r_dip(i);
  for (int i=0; i<3; i++) JDip(i) = J_dip(i);

  complex<double> ITo_G;
  blitz::TinyVector<std::complex<double>, 3> ITo_G_r, n_hat_X_ITo_G_r, ITo_G_r_rDip, ITo_grad_G, rDip_X_ITo_grad_G, r_p_X_ITo_grad_G, ITo_n_hat_X_r_X_grad_G, n_hat_X_r_p_X_ITo_grad_G;

  V_tE_J = 0.0; V_tH_J = 0.0;
  V_nE_J = 0.0; V_nH_J = 0.0;
  vector<HalfRWG> test_halfRWGs;
  for (int i=0 ; i<N_RWG_test ; ++i) {
    for (int j=0 ; j<2 ; ++j) {
      const int RWGnumber = numbers_RWG_test(i);
      const int triangle_number = abs(RWGNumber_signedTriangles(RWGnumber, j)), triangle_sign = (j==0) ? 1 : -1;
      blitz::Array<double, 1> r_opp(3);
      r_opp = (j==0) ? RWGNumber_oppVertexesCoord(RWGnumber, blitz::Range(0,2)) : RWGNumber_oppVertexesCoord(RWGnumber, blitz::Range(3,5));
      const double edge_length = RWGNumber_edgeLength(RWGnumber);
      test_halfRWGs.push_back(HalfRWG(RWGnumber, triangle_number, triangle_sign, edge_length, r_opp));
    }
  }
  sort(test_halfRWGs.begin(), test_halfRWGs.end());
  // vector of triangles construction
  vector<Triangle> triangles_test;
  constructVectorTriangles(triangles_test, test_halfRWGs, testTriangle_vertexesCoord);

  for (int r=0 ; r<triangles_test.size() ; ++r) { // loop on the observation triangles

    const Triangle T(triangles_test[r]);
    double R_os = sqrt (dot (triangles_test[r].r_grav - rDip, triangles_test[r].r_grav - rDip));
    bool IS_NEAR = (R_os - 1.5 * triangles_test[r].R_max <= 0.0);
    EXTRACT_1_R = (EXTRACT_R = 0); N_points = 6;
    if (IS_NEAR) {
      EXTRACT_1_R = (EXTRACT_R = 1); N_points = 9;
    }
    V_EH_ITo_free (ITo_G, ITo_G_r_rDip, ITo_grad_G, ITo_n_hat_X_r_X_grad_G, rDip, T, k, N_points, EXTRACT_1_R, EXTRACT_R);
    //ITs_free (ITs_G, ITs_G_r_rDip, ITs_grad_G, rDip, T, k, N_points, EXTRACT_1_R, EXTRACT_R);
    ITo_G_r = ITo_G_r_rDip + rDip * ITo_G;
    n_hat_X_ITo_G_r = T.n_hat(1)*ITo_G_r(2)-T.n_hat(2)*ITo_G_r(1),
                      T.n_hat(2)*ITo_G_r(0)-T.n_hat(0)*ITo_G_r(2),
                      T.n_hat(0)*ITo_G_r(1)-T.n_hat(1)*ITo_G_r(0);
    rDip_X_ITo_grad_G = rDip(1)*ITo_grad_G(2)-rDip(2)*ITo_grad_G(1),
                        rDip(2)*ITo_grad_G(0)-rDip(0)*ITo_grad_G(2),
                        rDip(0)*ITo_grad_G(1)-rDip(1)*ITo_grad_G(0);
    
    for (int p=0 ; p<triangles_test[r].halfRWGIndexes.size() ; ++p) {
      const int index_p = triangles_test[r].halfRWGIndexes[p];
      const int number_edge_p = test_halfRWGs[index_p].number;
      const int local_number_edge_p = test_halfRWGs[index_p].number;
      const double l_p = test_halfRWGs[index_p].length;
      const int sign_edge_p = test_halfRWGs[index_p].triangleSign;
      const double C_rp = sign_edge_p * l_p * 0.5/triangles_test[r].A;
      const TinyVector<double, 3> r_p(test_halfRWGs[index_p].r_opp_vertexesCoord);
      const TinyVector<double, 3> n_hat_X_r_p(cross(triangles_test[r].n_hat, r_p));
      r_p_X_ITo_grad_G = r_p(1)*ITo_grad_G(2)-r_p(2)*ITo_grad_G(1),
                         r_p(2)*ITo_grad_G(0)-r_p(0)*ITo_grad_G(2),
                         r_p(0)*ITo_grad_G(1)-r_p(1)*ITo_grad_G(0);

      n_hat_X_r_p_X_ITo_grad_G = n_hat_X_r_p(1)*ITo_grad_G(2)-n_hat_X_r_p(2)*ITo_grad_G(1),
                                 n_hat_X_r_p(2)*ITo_grad_G(0)-n_hat_X_r_p(0)*ITo_grad_G(2),
                                 n_hat_X_r_p(0)*ITo_grad_G(1)-n_hat_X_r_p(1)*ITo_grad_G(0);

      V_tE_J (local_number_edge_p) += -1.0/(I*w*eps*4.0*M_PI) * C_rp * dot(JDip, k*k*(ITo_G_r - r_p*ITo_G) + 2.0 * ITo_grad_G); // -<f_m ; E_inc> 
      V_nE_J (local_number_edge_p) += -1.0/(I*w*eps*4.0*M_PI) * C_rp * dot(JDip, k*k*(n_hat_X_ITo_G_r - n_hat_X_r_p*ITo_G) ); // -<n x f_m ; E_inc> 
      V_tH_J (local_number_edge_p) += C_rp/(4.0*M_PI) * dot(JDip, rDip_X_ITo_grad_G - r_p_X_ITo_grad_G); // -<f_m ; H_inc> 
      V_nH_J (local_number_edge_p) += C_rp/(4.0*M_PI) * dot(JDip, ITo_n_hat_X_r_X_grad_G - n_hat_X_r_p_X_ITo_grad_G); // -<f_m ; H_inc> 
    } // end for (p=0 ; p<3 ; p++)
  } // end for (m=0 ; m<N_triangles ; m++)
}
*/
