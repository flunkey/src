/* Imaging condition for shot-profile migration */
/*
  Copyright (C) 2007 Colorado School of Mines
  
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
  
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <math.h>

#include <rsf.h>
/*^*/

#include "img3.h"

#include "slice.h"
/*^*/

#define  LOOP(a) for( imy=0; imy< amy.n; imy++){ \
                 for( imx=0; imx< amx.n; imx++){ {a} }}

#define MLOOP(a) for( imz=0; imz< amz.n; imz++){ \
                 for( imy=0; imy< amy.n; imy++){ \
                 for( imx=0; imx< amx.n; imx++){ {a} }}}

#define HLOOP(a) for( ihz=LOz; ihz<HIz; ihz++){ \
                 for( ihy=LOy; ihy<HIy; ihy++){ \
                 for( ihx=LOx; ihx<HIx; ihx++){ {a} }}}

#define CLOOP(a) for( icz=0; icz< acz.n; icz++){ \
                 for( icy=0; icy< acy.n; icy++){ \
                 for( icx=0; icx< acx.n; icx++){ {a} }}}

#define XLOOP(a) for(imz  = abs(ihz); imz<amz.n-abs(ihz) ; imz++){ \
                     imzs = imz - ihz; \
                     imzr = imz + ihz; \
                 for(imy  = abs(ihy); imy<amy.n-abs(ihy) ; imy++){ \
                     imys = imy - ihy; \
                     imyr = imy + ihy; \
                 for(imx  = abs(ihx); imx<amx.n-abs(ihx) ; imx++){ \
                     imxs = imx - ihx; \
                     imxr = imx + ihx; \
                    {a} \
                 }}}

#define IND(ihx,ihy,ihz) (ihz-LOz)*(ahx.n*ahy.n) + \
                         (ihy-LOy)* ahx.n        + \
                         (ihx-LOx);

#define MM(i,a) SF_MIN(SF_MAX(i,0),a.n-1)

static sf_axa amx,amy,amz;
static sf_axa acx,acy,acz;
static int    jcx,jcy,jcz;
static sf_axa ahx,ahy,ahz;
static sf_axa aht;
static sf_axa ahh,aha,ahb;
static sf_axa aw;

static sf_complex   **tt; /* phase shift for time-lag I.C. */
static sf_complex ****qs;
static sf_complex ****qr;

static float       ***qi;

static float      ***qo;
static float   ******qx;
static float     ****qt;
static float     ****qh;

static int LOx,HIx;
static int LOy,HIy;
static int LOz,HIz;

static float vpvs;

/*------------------------------------------------------------*/
static float corr(sf_complex a, sf_complex b)
{
    sf_complex c;
#ifdef SF_HAS_COMPLEX_H
    c = conjf(a)*b;
#else
    c = sf_cmul(conjf(a),b);
#endif
    return crealf(c);
}

/*------------------------------------------------------------*/
static float wcorr(sf_complex a, sf_complex b, sf_complex w)
{
    sf_complex c;
#ifdef SF_HAS_COMPLEX_H
    c = conjf(a)*b*w;
#else
    c = sf_cmul(sf_cmul(conjf(a),b),w);
#endif
    return crealf(c);
}

/*------------------------------------------------------------*/
void img3o_init(sf_axis amx_,
		sf_axis amy_,
		sf_axis amz_,
		int jcx_,
		int jcy_,
		int jcz_,
		fslice imag,
		int ompnth
    )
/*< initialize zero-lag I.C. >*/
{
    int imx,imy,imz;
    int icx,icy,icz;
    int ompith;

    amx = sf_nod(amx_);
    amy = sf_nod(amy_);
    amz = sf_nod(amz_);

    jcx = jcx_; acx.n = amx.n / jcx;
    jcy = jcy_; acy.n = amy.n / jcy;
    jcz = jcz_; acz.n = amz.n / jcz;

    /* allocate wavefield storage */
    qs = sf_complexalloc4(amx.n,amy.n,amz.n,ompnth);
    qr = sf_complexalloc4(amx.n,amy.n,amz.n,ompnth);
    for(ompith=0; ompith<ompnth; ompith++){
	MLOOP( qs[ompith][imz][imy][imx] = sf_cmplx(0.0,0.0); 
	       qr[ompith][imz][imy][imx] = sf_cmplx(0.0,0.0); );
    }

    /* allocate image storage */
    qi = sf_floatalloc3(amx.n,amy.n,amz.n);
    MLOOP( qi[imz][imy][imx] = 0.0; );

    /* allocate cigs storage */
    qo = sf_floatalloc3(acx.n,acy.n,acz.n);
    CLOOP( qo[icz][icy][icx] = 0.0; );
}

/*------------------------------------------------------------*/
void img3x_init(sf_axis amx_,
		sf_axis amy_,
		sf_axis amz_,
		int jcx_,
		int jcy_,
		int jcz_,
		sf_axis ahx_,
		sf_axis ahy_,
		sf_axis ahz_,
		fslice imag,
		int ompnth
    )
/*< initialize x-lag I.C. >*/
{
    int imx,imy,imz;
    int icx,icy,icz;
    int ihx,ihy,ihz;
    int ompith;

    amx = sf_nod(amx_);
    amy = sf_nod(amy_);
    amz = sf_nod(amz_);

    jcx = jcx_; acx.n = amx.n / jcx;
    jcy = jcy_; acy.n = amy.n / jcy;
    jcz = jcz_; acz.n = amz.n / jcz;
    
    ahx = sf_nod(ahx_);
    ahy = sf_nod(ahy_);
    ahz = sf_nod(ahz_);

    LOx = floor(ahx.o/ahx.d); HIx = LOx + ahx.n;
    LOy = floor(ahy.o/ahy.d); HIy = LOy + ahy.n;
    LOz = floor(ahz.o/ahz.d); HIz = LOz + ahz.n;

    /* allocate wavefield storage */
    qs = sf_complexalloc4(amx.n,amy.n,amz.n,ompnth);
    qr = sf_complexalloc4(amx.n,amy.n,amz.n,ompnth);
    for(ompith=0; ompith<ompnth; ompith++){
	MLOOP( qs[ompith][imz][imy][imx] = sf_cmplx(0.0,0.0); 
	       qr[ompith][imz][imy][imx] = sf_cmplx(0.0,0.0); );
    }

    /* allocate image storage */
    qi = sf_floatalloc3(amx.n,amy.n,amz.n);
    MLOOP( qi[imz][imy][imx] = 0.0; );

    /* allocate cigs storage */
    qx = sf_floatalloc6(acx.n,acy.n,acz.n,ahx.n,ahy.n,ahz.n);
    HLOOP(
	CLOOP( 
	    qx[ihz-LOz][ihy-LOy][ihx-LOx][icz][icy][icx] = 0.0; 
	    );
	);
}

/*------------------------------------------------------------*/
void img3t_init(sf_axis amx_,
		sf_axis amy_,
		sf_axis amz_,
		int jcx_,
		int jcy_,
		int jcz_,
		sf_axis aht_,
		sf_axis aw_,
		fslice imag,
		int ompnth
    )
/*< initialize t-lag I.C. >*/
{
    int imx,imy,imz;
    int icx,icy,icz;
    int  iht,iw;
    float ht, w;
    int ompith;

    amx = sf_nod(amx_);
    amy = sf_nod(amy_);
    amz = sf_nod(amz_);

    jcx = jcx_; acx.n = amx.n / jcx;
    jcy = jcy_; acy.n = amy.n / jcy;
    jcz = jcz_; acz.n = amz.n / jcz;
    
    aht = sf_nod(aht_);
    aw  = sf_nod(aw_);

    /* allocate wavefield storage */
    qs = sf_complexalloc4(amx.n,amy.n,amz.n,ompnth);
    qr = sf_complexalloc4(amx.n,amy.n,amz.n,ompnth);
    for(ompith=0; ompith<ompnth; ompith++){
	MLOOP( qs[ompith][imz][imy][imx] = sf_cmplx(0.0,0.0); 
	       qr[ompith][imz][imy][imx] = sf_cmplx(0.0,0.0); );
    }

    /* allocate image storage */
    qi = sf_floatalloc3(amx.n,amy.n,amz.n);
    MLOOP( qi[imz][imy][imx] = 0.0; );
    
    /* allocate cigs storage */
    qt = sf_floatalloc4(acx.n,acy.n,acz.n,aht.n);
    for( iht=0; iht<aht.n; iht++) {
	CLOOP( qt[iht][icz][icy][icx] = 0.0; );
    }

    /* from hertz to radian */
    aw.d *= 2.*SF_PI; 
    aw.o *= 2.*SF_PI;

    /* precompute phase shift */
    tt = sf_complexalloc2(aht.n,aw.n);
    for (iw=0; iw<aw.n; iw++) {
	w = aw.o+iw*aw.d;
	for (iht=0; iht<aht.n; iht++) {
	    ht = aht.o+iht*aht.d;
	    ht *= -2*w;
	    tt[iw][iht] = sf_cmplx(cosf(ht),sinf(ht));
	}
    }
}

/*------------------------------------------------------------*/
void img3h_init(sf_axis amx_,
		sf_axis amy_,
		sf_axis amz_,
		int jcx_,
		int jcy_,
		int jcz_,
		sf_axis ahh_,
		sf_axis aha_,
		sf_axis ahb_,
		sf_axis aw_,
		fslice imag,
		float vpvs_,
		int ompnth
    )
/*< initialize abs x-lag I.C. >*/
{
    int imx,imy,imz;
    int icx,icy,icz;
    int  ihh;
    int ompith;

    amx = sf_nod(amx_);
    amy = sf_nod(amy_);
    amz = sf_nod(amz_);

    jcx = jcx_; acx.n = amx.n / jcx;
    jcy = jcy_; acy.n = amy.n / jcy;
    jcz = jcz_; acz.n = amz.n / jcz;
    
    ahh = sf_nod(ahh_);
    aha = sf_nod(aha_);
    ahb = sf_nod(ahb_);
    aw  = sf_nod(aw_);

    vpvs = vpvs_;

    /* allocate wavefield storage */
    qs = sf_complexalloc4(amx.n,amy.n,amz.n,ompnth);
    qr = sf_complexalloc4(amx.n,amy.n,amz.n,ompnth);
    for(ompith=0; ompith<ompnth; ompith++){
	MLOOP( qs[ompith][imz][imy][imx] = sf_cmplx(0.0,0.0); 
	       qr[ompith][imz][imy][imx] = sf_cmplx(0.0,0.0); );
    }

    /* allocate image storage */
    qi = sf_floatalloc3(amx.n,amy.n,amz.n);
    MLOOP( qi[imz][imy][imx] = 0.0; );
    
    /* allocate cigs storage */
    qh = sf_floatalloc4(acx.n,acy.n,acz.n,ahh.n);
    for( ihh=0; ihh<ahh.n; ihh++) {
	CLOOP( qh[ihh][icz][icy][icx] = 0.0; );
    }
}

/*------------------------------------------------------------*/
void img3o(int iw,
	   int ompith)
/*< apply zero-lag I.C. >*/
{
    int imx,imy,imz;
    int icx,icy,icz;

    /* imag */
    MLOOP(
	;    qi        [imz][imy][imx] +=
	corr(qs[ompith][imz][imy][imx],    
	     qr[ompith][imz][imy][imx]);
	);

    /* cigs */
    CLOOP(
	;    qo        [icz    ][icy    ][icx    ] +=
	corr(qs[ompith][icz*jcz][icy*jcy][icx*jcx], 
	     qr[ompith][icz*jcz][icy*jcy][icx*jcx]);
	);
}

/*------------------------------------------------------------*/
void img3x(int iw,
	   int ompith)
/*< apply x-lag I.C. >*/
{
    int imx, imy, imz;
    int icx, icy, icz;
    int ihx, ihy, ihz;
    int imys,imyr,imzs;
    int imxs,imxr,imzr;

    /* imag */
    MLOOP(
	;    qi        [imz][imy][imx] +=
	corr(qs[ompith][imz][imy][imx], 
	     qr[ompith][imz][imy][imx]);
	);

    /* cigs */
/*    HLOOP(*/
/*	   XLOOP(*/
/*	       ;             qo[imz ][imy ][imx ] =*/
/*	       crealf( conjf(qs[imzs][imys][imxs]) */
/*		       *     qr[imzr][imyr][imxr]);*/
/*	       );*/
/*	   CLOOP(*/
/*	       qx[ihz-LOz][ihy-LOy][ihx-LOx][icz    ][icy    ][icx    ] +=*/
/*	       qo                           [icz*jcz][icy*jcy][icx*jcx];*/
/*	       );*/
/*	);*/

    
    for( ihz=LOz; ihz<HIz; ihz++){ 
	for( ihy=LOy; ihy<HIy; ihy++){
	    for( ihx=LOx; ihx<HIx; ihx++){

		for( icz=0; icz<acz.n; icz++){ 
		    imzs = icz*jcz - ihz;
		    imzr = icz*jcz + ihz;
		    if(imzs>=0 && imzs<amz.n && 
		       imzr>=0 && imzr<amz.n) {
			for( icy=0; icy<acy.n; icy++){
			    imys = icy*jcy - ihy;
			    imyr = icy*jcy + ihy;
			    if(imys>=0 && imys<amy.n && 
			       imyr>=0 && imyr<amy.n) {

			for( icx=0; icx<acx.n; icx++){
				    imxs = icx*jcx - ihx;
				    imxr = icx*jcx + ihx;
				    if(imxs>=0 && imxs<amx.n && 
				       imxr>=0 && imxr<amx.n) {

					qx[ihz-LOz][ihy-LOy][ihx-LOx]
					    [icz][icy][icx] +=
					    corr(qs[ompith][imzs][imys][imxs], 
						 qr[ompith][imzr][imyr][imxr]);
				    }
				}
			    }
			}
		    }
		}		
	    }
	}
    }
}

/*------------------------------------------------------------*/
void img3t(int iw,
	   int ompith)
/*< apply t-lag I.C. >*/
{
    int imx,imy,imz,iht;
    int icx,icy,icz;
    sf_complex wt;

    /* imag */
    MLOOP(
	;    qi        [imz][imy][imx] +=
	corr(qs[ompith][imz][imy][imx], 
	     qr[ompith][imz][imy][imx]);
	);

    /* cigs */
    for(iht=0; iht<aht.n; iht++) {
	wt = tt[iw][iht];
	
	CLOOP(;     qt        [iht][icz    ][icy    ][icx    ] += 
	      wcorr(qs[ompith]     [icz*jcz][icy*jcy][icx*jcx], 
		    qr[ompith]     [icz*jcz][icy*jcy][icx*jcx],
		    wt); ); 
    }
}

/*------------------------------------------------------------*/
void img3h(int iw,
	   int ompith)
/*< apply abs x-lag I.C. >*/
{
    int imx,imy,imz;
    int ihh,iha,ihb;
    int icx,icy,icz;

    int dsx,dsy,dsz;
    int drx,dry,drz;

    int isx,isy,isz;
    int irx,iry,irz;

    float hh,aa,bb; /* aa,bb in radians */
    float hx,hy,hz;
    sf_complex cs,cr;
    float hscale;

    /* imag */
    MLOOP(
	;    qi        [imz][imy][imx] +=
	corr(qs[ompith][imz][imy][imx],
	     qr[ompith][imz][imy][imx] );
	);

    /* cigs */
    for(ihh=0; ihh<ahh.n; ihh++) {                /* absolute offset */
	hh = ahh.o + ihh * ahh.d;

	hscale=1.;
	if(ihh>0) hscale=(hh+ahh.d)*aha.d;

	for(ihb=0; ihb<ahb.n; ihb++) {        /* latitude  */
	    bb = ahb.o + ihb * ahb.d;
	    for(iha=0; iha<aha.n; iha++) {    /* longitude */
		aa = aha.o + iha * aha.d;
		
		hz = hh * sin(aa);
		hx = hh * cos(aa) * cos(bb);
		hy = hh * cos(aa) * sin(bb);

		/* nearest neighbour - source dh */
		dsx = (int)( (2.*vpvs/(1.+vpvs))*hx / amx.d);
		dsy = (int)( (2.*vpvs/(1.+vpvs))*hy / amy.d);
		dsz = (int)( (2.*vpvs/(1.+vpvs))*hz / amz.d);

		/* nearest neighbour - receiver dh */
		drx = (int)( (2.*  1./(1.+vpvs))*hx / amx.d);
		dry = (int)( (2.*  1./(1.+vpvs))*hy / amy.d);
		drz = (int)( (2.*  1./(1.+vpvs))*hz / amz.d);

		CLOOP(
		    isx=MM(icx*jcx-dsx,amx); 
		    isy=MM(icy*jcy-dsy,amy);
		    isz=MM(icz*jcz-dsz,amz);
		    cs = qs[ompith][isz][isy][isx];

		    irx=MM(icx*jcx+dsx,amx);
		    iry=MM(icy*jcy+dsy,amy);
		    irz=MM(icz*jcz+dsz,amz);
		    cr = qr[ompith][irz][iry][irx];

		    /* 
		       cs = qs @ x-hx,y-hy,z-hz
		       cr = qr @ x+hx,y+hy,z+hz
		    */
		    qh[ihh][icz][icy][icx] += hscale * corr(cs,cr);  
		    ); /* cx,cy,cz */

	    } /* aa */
	}     /* bb */	
    }         /* hh */
}

/*------------------------------------------------------------*/
void img3o_close(fslice imag,
		 fslice cigs)
/*< deallocate zero-lag I.C. >*/
{
    fslice_put(imag,0,qi[0][0]);
    fslice_put(cigs,0,qo[0][0]);

    img3_close();

    free(**qo); 
    free( *qo); 
    free(  qo);
}

/*------------------------------------------------------------*/
void img3t_close(fslice imag,
		 fslice cigs)
/*< deallocate t-lag I.C. >*/
{
    fslice_put(imag,0,qi[0][0]);
    fslice_put(cigs,0,qt[0][0][0]);

    img3_close();

    free(***qt); 
    free( **qt); 
    free(  *qt); 
    free(   qt);

    free(*tt); 
    free( tt);
}

/*------------------------------------------------------------*/
void img3h_close(fslice imag,
		 fslice cigs)
/*< deallocate abs x-lag I.C. >*/
{
    fslice_put(imag,0,qi[0][0]);
    fslice_put(cigs,0,qh[0][0][0]);

    img3_close();

    free(***qh); 
    free( **qh); 
    free(  *qh); 
    free(   qh);
}

/*------------------------------------------------------------*/
void img3x_close(fslice imag,
		 fslice cigs)
/*< deallocate x-lag I.C. >*/
{
    fslice_put(imag,0,qi[0][0]);
    fslice_put(cigs,0,qx[0][0][0][0][0]);

    img3_close();

    free(*****qx); 
    free( ****qx); 
    free(  ***qx); 
    free(   **qx); 
    free(    *qx); 
    free(     qx);
}

/*------------------------------------------------------------*/
void img3_close()
/*< deallocate >*/
{
    free(***qs);free(**qs); free(*qs); free(qs);
    free(***qr);free(**qr); free(*qr); free(qr);
    ;           free(**qi); free(*qi); free(qi);
}

/*------------------------------------------------------------*/
void img3store( int imz,
		sf_complex ***ww_s,
		sf_complex ***ww_r,
		int ompith
    )
/*< store wavefield >*/
{
    int imx,imy;

    LOOP( qs[ompith][imz][imy][imx] = ww_s[ompith][imy][imx];
	  qr[ompith][imz][imy][imx] = ww_r[ompith][imy][imx]; );
}

