#ifndef _VECMATOP_HH_
#define _VECMATOP_HH_

#include "nummat.hh"

using std::vector;
//typedef long int int;

//--------------------------------------------------
int dgemm(float alpha, const FltNumMat& A, const FltNumMat& B, float beta, FltNumMat& C);
int dgemm(int m, int n, int k, float alpha, float* A, float* B, float beta, float* C);

int dgemv(float alpha, const FltNumMat& A, const FltNumVec& X, float beta, FltNumVec& Y);
int dgemv(int m, int n, float alpha, float* A, float* X, float beta, float* Y);

//--------------------------------------------------
int zgemm(cpx alpha, const CpxNumMat& A, const CpxNumMat& B, cpx beta, CpxNumMat& C);
int zgemm(int m, int n, int k, cpx alpha, cpx* A, cpx* B, cpx beta, cpx* C);

int zgemv(cpx alpha, const CpxNumMat& A, const CpxNumVec& X, cpx beta, CpxNumVec& Y);
int zgemv(int m, int n, cpx alpha, cpx* A, cpx* X, cpx beta, cpx* Y);

//--------------------------------------------------
int dgmres(int (*A)(const FltNumVec&, FltNumVec&), const FltNumVec& b, const FltNumVec& x0,
	   int restart, float tol, int maxit, int print,
	   FltNumVec& x, int& flag, float& relres, int& iter, vector<float>& resvec);

int zgmres(int (*A)(const CpxNumVec&, CpxNumVec&), const CpxNumVec& b, const CpxNumVec& x0,
	   int restart, float tol, int maxit, int print,
	   CpxNumVec& x, int& flag, float& relres, int& iter, vector<float>& resvec);

//--------------------------------------------------
int pinv(const FltNumMat& M, float eps, FltNumMat& R);

//--------------------------------------------------
int lowrank(int m, int n, int (*sample)(vector<int>&, vector<int>&, FltNumMat&), float eps, int npk,
	    vector<int>& cidx, vector<int>& ridx, FltNumMat& mid);

int ztran(const CpxNumMat& A, CpxNumMat& B);

#endif

