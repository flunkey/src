// Lowrank matrix decomposition

//   Copyright (C) 2010 University of Texas at Austin
//  
//   This program is free software; you can redistribute it and/or modify
//   it under the terms of the GNU General Public License as published by
//   the Free Software Foundation; either version 2 of the License, or
//   (at your option) any later version.
//  
//   This program is distributed in the hope that it will be useful,
//   but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//   GNU General Public License for more details.
//  
//   You should have received a copy of the GNU General Public License
//   along with this program; if not, write to the Free Software
//   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#include <time.h>

#include <rsf.hh>

#include "vecmatop.hh"
#include "serialize.hh"

using namespace std;

static FltNumMat matrix;

int sample(vector<int>& rs, vector<int>& cs, FltNumMat& res)
{
    int nr = rs.size();
    int nc = cs.size();
    res.resize(nr,nc);  
    setvalue(res,0.0f);
    for(int a=0; a<nr; a++) {
	for(int b=0; b<nc; b++) {
	    res(a,b) = matrix(rs[a],cs[b]);
	}
    }
    return 0;
}

static void output(const char* name, FltNumMat& mat)
{
    int m=mat.m();
    int n=mat.n();
    int mn = m*n;

    float *dmat = mat.data();

    std::valarray<float> fmat(mn);
    for (int k=0; k < mn; k++) {
	fmat[k] = dmat[k];
    }
	
    oRSF mfile(name);
    mfile.put("n1",m);
    mfile.put("n2",n);
    mfile << fmat;
}

int main(int argc, char** argv)
{   
    sf_init(argc,argv); // Initialize RSF

    iRSF par(0);
    int seed;

    par.get("seed",seed,time(NULL)); // seed for random number generator
    srand48(seed);

    float eps;
    par.get("eps",eps,1.e-4); // tolerance

    int npk;
    par.get("npk",npk,20); // maximum rank

    int outputs;
    par.get("outputs",outputs,3); // number of outputs (2 or 3)

    iRSF in;
    oRSF out;

    int m,n;
    in.get("n1",m);
    in.get("n2",n);
    std::valarray<float> fdata(m*n);
    in >> fdata;
    
    std::valarray<float> data(m*n);
    for (int k=0; k < m*n; k++) {
	data[k] = fdata[k];
    }
    
    FltNumMat mat(m,n,false,&(data[0]));
    matrix = mat;
    
    vector<int> midx(m), nidx(n), lidx, ridx;
    FltNumMat mid;

    iC( lowrank(m,n,sample,eps,npk,lidx,ridx,mid) );

    int m2=mid.m();
    int n2=mid.n();

    for (int k=0; k < m; k++) 
	midx[k] = k;
    for (int k=0; k < n; k++) 
	nidx[k] = k;    

    FltNumMat lmat(m,m2);
    iC ( sample(midx,lidx,lmat) );

    FltNumMat rmat(n2,n);
    iC ( sample(ridx,nidx,rmat) );

    FltNumMat tmp1(m,n2);
    iC( dgemm(1.0, lmat, mid, 0.0, tmp1) );

    if (outputs > 2) {
	output("mid",mid);
	output("left",lmat);
    } else {
	output("left",tmp1);
    }

    output("right",rmat);

    iC( dgemm(1.0, tmp1, rmat, 0.0, mat) );
    
    for (int k=0; k < m*n; k++) 
	fdata[k] = data[k];
    out << fdata;

    return 0;
}





