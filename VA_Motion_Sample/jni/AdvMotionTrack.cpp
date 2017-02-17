#ifdef LINUX
#  include <stdio.h>
#  include <sys/time.h>
#  include <geo_config.h>
#endif

#include <sys/time.h>

#ifdef WIN32
#  include <windows.h>
#endif


#include <memory.h>
#include <stdlib.h>
#include "AdvMotionTrack.h"

/// Get n-th byte from 32bits integer. zero based index
#if (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
#define GET_NTH_BYTE(x,n) (((x) >> (n*8)) & 0xFF)
#else
#define GET_NTH_BYTE(x,n) (((x) >> (24 - n*8)) & 0xFF)
#endif


#define GV_SHOW
#define posix_memalign
#define GV_ASSERT

/// use inline function decrease function call cost
inline int myabs(register int iNum)
{
    register int temp = iNum >> 31;

    iNum = iNum ^ temp;
    iNum = iNum - temp;

    return iNum;
}

CAdvMotionTrack::CAdvMotionTrack(int nWidth,int nHeight,int nSensitivity,bool bDenoise, int is_USE_CIF_Detect)
{
    int i, scale_V;

    GV_SHOW ("nWidth[%d], nHeight[%d], nSensitivity[%d], bDenoise[%d]\n",
            nWidth, nHeight, nSensitivity, bDenoise);
    m_bDenoise = bDenoise;
    m_nSensitivity = nSensitivity;
    m_nSensitivity = (m_nSensitivity<0) ? 0 : m_nSensitivity;
    m_nSensitivity = (m_nSensitivity>=10) ? 9 : m_nSensitivity;
    m_dTickInterval = 200;
    m_nHScale = m_nH = nWidth;
    m_nVScale = m_nV = nHeight;
    m_nScaleLevel = 1;

    if (is_USE_CIF_Detect)
    { scale_V = 384; }
    else
    { scale_V = 192; }

    /// In 16:9, 640x384 -> 320x192 -> 160x96
    while(m_nVScale >= scale_V) {
        m_nHScale /= 2;
        m_nVScale /= 2;
        m_nScaleLevel *= 2;
    }
    m_pSensitivity = new unsigned char[m_nHScale*m_nVScale];
    memset(m_pSensitivity,m_nSensitivity,m_nHScale*m_nVScale);
    m_pMask = new unsigned char[m_nHScale*m_nVScale];
    posix_memalign((void **)&m_pMask, 32, m_nHScale*m_nVScale);
    memset(m_pMask,1,m_nHScale*m_nVScale);
    m_pIdiff = new unsigned char[m_nHScale*m_nVScale];
    m_pIdiff1 = new unsigned char[m_nHScale*m_nVScale];
    m_pIdiff2 = new unsigned char[m_nHScale*m_nVScale];
    m_pYDataScale = new unsigned char[m_nHScale*m_nVScale];
    posix_memalign((void **)&m_pIdiff, 32, m_nHScale*m_nVScale);
    posix_memalign((void **)&m_pIdiff1, 32, m_nHScale*m_nVScale);
    posix_memalign((void **)&m_pIdiff2, 32, m_nHScale*m_nVScale);
    posix_memalign((void **)&m_pYDataScale, 32, m_nHScale*m_nVScale);
    m_pIntegral = new int[m_nHScale*m_nVScale];
    m_pLabel = new short[m_nHScale*m_nVScale];
    memset (m_pLabel, 0, sizeof (short) * (m_nHScale*m_nVScale) );
    m_ppLabelRec = new short*[m_nHScale*m_nVScale];
    m_ppLabelRec[0] = new short[m_nHScale*m_nVScale*3];
    for(i=1;i<m_nHScale*m_nVScale;i++) {
        m_ppLabelRec[i] = m_ppLabelRec[i-1]+3;
    }
    m_pnBuffer = new int[m_nHScale*4];
    memset(m_pIdiff,0,m_nHScale*m_nVScale);
    memset(m_pIdiff1,0,m_nHScale*m_nVScale);
    memset(m_pIdiff2,0,m_nHScale*m_nVScale);
    memset(m_pIntegral,0,m_nHScale*m_nVScale*sizeof(int));
    m_nTd = 20;
    m_bHasValidObject = false;
    m_bFirstVideo = m_bFirstMask = true;
    m_dTickRec = m_dTickRecRef = 0;
    m_nValidObjectSize[9] = 2;
    m_nValidObjectSize[8] = 4;
    m_nValidObjectSize[7] = 7;
    m_nValidObjectSize[6] = 10;
    m_nValidObjectSize[5] = 13;
    m_nValidObjectSize[4] = 16;
    m_nValidObjectSize[3] = 19;
    m_nValidObjectSize[2] = 22;
    m_nValidObjectSize[1] = 25;
    m_nValidObjectSize[0] = 28;
}

CAdvMotionTrack::~CAdvMotionTrack()
{
    GV_SHOW ("[1]\n");
    delete[] m_pSensitivity;
    free(m_pMask);
    GV_SHOW ("[2]\n");
    free(m_pIdiff);
    free(m_pIdiff1);
    free(m_pIdiff2);
    GV_SHOW ("[3]\n");
    free(m_pYDataScale);
    GV_SHOW ("[5]\n");
    delete[] m_pIntegral;
    delete[] m_pLabel;
    GV_SHOW ("[6]\n");
    delete[] m_ppLabelRec[0];
    delete[] m_ppLabelRec;
    GV_SHOW ("[7]\n");
    delete[] m_pnBuffer;
    GV_SHOW ("[8]\n");
}

void CAdvMotionTrack::AddMotionArea(int nL,int nT,int nR,int nB,int nWidthBase,int nHeightBase,int nSensitivity)
{
    int i,j;
    unsigned char *ptsrc,*ptsrcMask;

    if (nL < 0) nL = 0;
    if (nL >= nWidthBase) nL = nWidthBase - 1;
    if (nR < 0) nL = 0;
    if (nR >= nWidthBase) nR = nWidthBase - 1;
    if (nT < 0) nT = 0;
    if (nT >= nHeightBase) nT = nHeightBase - 1;
    if (nB < 0) nB = 0;
    if (nB >= nHeightBase) nB = nHeightBase - 1;

    if(m_bFirstMask == true) {
        m_bFirstMask = false;
        memset(m_pMask,0,m_nHScale*m_nVScale);
    }
    nSensitivity = (nSensitivity<0) ? 0 : nSensitivity;
    nSensitivity = (nSensitivity>=10) ? 9 : nSensitivity;
    nL = nL*m_nHScale/nWidthBase;
    nR = nR*m_nHScale/nWidthBase;
    nT = nT*m_nVScale/nHeightBase;
    nB = nB*m_nVScale/nHeightBase;
    GV_SHOW ("m_nHVScale[%dx%d], n[%d, %d, %d, %d], Base[%dx%d]\n",
            m_nHScale, m_nVScale, nT, nB, nL, nR, nWidthBase, nHeightBase);

    for(i=nT;i<nB;i++) {
        ptsrc = m_pSensitivity + i * m_nHScale;
        ptsrcMask = m_pMask + i * m_nHScale;
        for(j=nL;j<nR;j++) {
            GV_ASSERT (ptsrc + j < m_pSensitivity + m_nHScale*m_nVScale, "FIXME:i[%d], j[%d]\n", i, j);
            if (ptsrc + j >= m_pSensitivity + m_nHScale*m_nVScale)
            { continue; }
            ptsrc[j] = nSensitivity;
            ptsrcMask[j] = 1;
        }
    }
}

void CAdvMotionTrack::DelMotionArea(int nL,int nT,int nR,int nB,int nWidthBase,int nHeightBase)
{
    int i,j;
    unsigned char *ptsrc,*ptsrcMask;

    if(m_bFirstMask == true) {
        m_bFirstMask = false;
        memset(m_pMask,0,m_nHScale*m_nVScale);
    }
    nL = nL*m_nHScale/nWidthBase;
    nR = nR*m_nHScale/nWidthBase;
    nT = nT*m_nVScale/nHeightBase;
    nB = nB*m_nVScale/nHeightBase;
    ptsrc = m_pSensitivity+nT*m_nHScale;
    ptsrcMask = m_pMask+nT*m_nHScale;
    for(i=nT;i<nB;i++) {
        for(j=nL;j<nR;j++) {
            ptsrc[j] = 0;
            ptsrcMask[j] = 0;
        }
        ptsrc += m_nHScale;
        ptsrcMask += m_nHScale;
    }
}

void CAdvMotionTrack::DoHorizBlur(unsigned char *src, int src_w, int src_h, unsigned char *dst, int boxw)
{
    if(boxw < 0) {
        memcpy(dst,src,src_w*src_h);
        return;
    }
    if(boxw >= src_w) {
        boxw=src_w-1;
    }

    int y,x,tot;
    int mul = _FIX(boxw*2+1);
    unsigned char cValue, *ptsrc = src, *ptdst = dst;

    for(y=0;y<src_h;y++) {
        tot = 0;
        for(x=0;x<boxw;x++) {
            tot += ptsrc[x];
        }
        for(x=0;x<src_w;x++) {
            if(x > boxw) {
                tot -= ptsrc[-boxw-1];
            }
            if(x+boxw < src_w) {
                tot += ptsrc[boxw];
            }
            *ptdst++ = ((tot*mul)>>_SCALEBITS);
            ptsrc++;
        }
    }
    ptdst = dst;
    for(y=0;y<src_h;y++) {
        cValue = ptdst[boxw];
        memset(ptdst,cValue,boxw);
        ptdst += src_w;
    }
    ptdst = dst+src_w-boxw;
    for(y=0;y<src_h;y++) {
        cValue = ptdst[-1];
        memset(ptdst,cValue,boxw);
        ptdst += src_w;
    }
}

void CAdvMotionTrack::DoVerticalBlur(unsigned char *src, int src_w, int src_h, unsigned char *dst, int boxw)
{
    if(boxw < 0) {
        memcpy(dst,src,src_w*src_h);
        return;
    }
    if(boxw >= src_h) {
        boxw = src_h-1;
    }
    int y,x;
    register int tot, tot2, tot3, tot4;
    int mul = _FIX(boxw*2+1);
    unsigned char *ptsrc = src, *ptdst = dst;

    register unsigned long rSrc_3 = 0;
    register unsigned long rSrc_2 = 0;
    register unsigned long rSrc_1 = 0;
    register unsigned long rSrc_0 = 0;

    /// We process 4 pixels in one time
    for(x=0; x < src_w; x += 4) {

        tot = tot2 = tot3 = tot4 = 0;

        for(y = 0; y < boxw; y++) {

            rSrc_0 = *((unsigned long *)(ptsrc + (x + y*src_w)));

            tot += GET_NTH_BYTE(rSrc_0, 0);
            tot2 += GET_NTH_BYTE(rSrc_0, 1);
            tot3 += GET_NTH_BYTE(rSrc_0, 2);
            tot4 += GET_NTH_BYTE(rSrc_0, 3);
        }

        for(y = 0; y < src_h; y++) {

            register unsigned long rDst = 0;

            if(y > boxw) {
                rSrc_3 = *(unsigned long *)(ptsrc + x+(y-boxw-1)*src_w);
                tot -= GET_NTH_BYTE(rSrc_3, 0);
                tot2 -= GET_NTH_BYTE(rSrc_3, 1);
                tot3 -= GET_NTH_BYTE(rSrc_3, 2);
                tot4 -= GET_NTH_BYTE(rSrc_3, 3);
            }

            if(y + boxw < src_h) {
                rSrc_0 = *(unsigned long *)(ptsrc + x+(y+boxw)*src_w);

                tot += GET_NTH_BYTE(rSrc_0, 0);
                tot2 += GET_NTH_BYTE(rSrc_0, 1);
                tot3 += GET_NTH_BYTE(rSrc_0, 2);
                tot4 += GET_NTH_BYTE(rSrc_0, 3);
            }
#if (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
            rDst = ((tot*mul)>>_SCALEBITS) | (((tot2*mul)>>_SCALEBITS) << 8) | (((tot3*mul)>>_SCALEBITS) << 16) | (((tot4*mul)>>_SCALEBITS) << 24);
#else
            rDst = ((tot*mul)>>_SCALEBITS) << 24 | (((tot2*mul)>>_SCALEBITS) << 16) | (((tot3*mul)>>_SCALEBITS) << 8) | (((tot4*mul)>>_SCALEBITS));
#endif

            *(unsigned long *)(ptdst + y*src_w+x) = rDst;

            rSrc_3 = rSrc_2;
            rSrc_2 = rSrc_1;
            rSrc_1 = rSrc_0;
        }
    }
    ptsrc = dst+boxw*src_w;
    ptdst = dst;
    for(y=0;y<boxw;y++) {
        memcpy(ptdst,ptsrc,src_w);
        ptdst += src_w;
    }
    ptsrc = dst+(src_h-1-boxw)*src_w;
    ptdst = dst+(src_h-boxw)*src_w;
    for(y=0;y<boxw;y++) {
        memcpy(ptdst,ptsrc,src_w);
        ptdst += src_w;
    }
}

void CAdvMotionTrack::ScaledownYUY2(unsigned char *pIn,unsigned char *pOut,int nWidth,int nHeight,int nLevel)
{
    int i,j;
    int nHSkipByte = nLevel*2;
    int nVSkipByte = (nLevel-1)*nWidth*2;

    for(i=0;i<nHeight;i+=nLevel) {
        for(j=0;j<nWidth;j+=nLevel) {
            *pOut++ = *pIn;
            pIn += nHSkipByte;
        }
        pIn += nVSkipByte;
    }
}

void CAdvMotionTrack::ScaledownY(unsigned char *pIn,unsigned char *pOut,int nWidth,int nHeight,int nLevel)
{
    register unsigned long v, w;
    int i,j;
    int nHSkipByte = nLevel;
    int nVSkipByte = (nLevel-1)*nWidth;

    if(nLevel == 4) {

        unsigned long * pOut2 = (unsigned long *)pOut;
        unsigned char * pIn2 = (unsigned char *)pIn;

        for(i=0; i < nHeight; i+= nLevel) {

            /// One line scale
            for(j=0; j < nWidth; j += 16) {
#if (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
                *pOut2 = pIn2[0] | (pIn2[4] << 8) | (pIn2[8] << 16) | (pIn2[12] << 24);
#else
                *pOut2 = (pIn2[0] << 24) | (pIn2[4] << 16) | (pIn2[8] << 8) | (pIn2[12]);
#endif

                pOut2++;         /// next 4 bytes address for writing
                pIn2 += 16;      /// next 16 bytes address for reading
            }

            /// Jump to next line for scale
            pIn2 = ((unsigned char *)pIn2 + nVSkipByte);
        }
    }
    else if(nLevel == 2) {
        unsigned long * pOut2 = (unsigned long *)pOut;
        unsigned long * pIn2 = (unsigned long *)pIn;

        for(i=0; i < nHeight; i+= nLevel) {

            /// One line scale
            for(j=0; j < nWidth; j += 8) {
                v = *pIn2;      /// The first 4 bytes
                w = *(pIn2+1);  /// The next 4 bytes
#if (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
                *pOut2 = (v & 0x000000FF) | ((v >> 8) & 0x0000FF00) | ((w & 0x000000FF) << 16) | ((w >> 16) << 24);
#else
                *pOut2 = (v & 0xFF000000) | (((v >> 8) & 0x000000FF) << 16) | (((w >> 24) & 0xFF) << 8) | ((w >> 8) & 0xFF));
#endif

                pOut2++;        /// next 4 bytes address for writing
                pIn2 += 2;      /// next 8 bytes address for reading
            }     /// Jump to next line for scale
            pIn2 = (unsigned long *)((unsigned char *)pIn2 + nVSkipByte);
        }
    }
    else if(nLevel == 1) {
        /// 1:1 copy
        memcpy(pOut, pIn, nWidth * nHeight);
    }
    else {
        for(i=0;i<nHeight;i+=nLevel) {
            for(j=0;j<nWidth;j+=nLevel) {

                *pOut++ = *pIn;
                pIn += nHSkipByte;
            }
            pIn += nVSkipByte;
        }
    }
}

void CAdvMotionTrack::ObjectLabeling(unsigned char *pIn,short *pOut,int nH,int nV)
{
    bool bCheck;
    short *ptdst,*ptdst1,nBuf[4];
    unsigned char *ptsrc;
    int	i,j,k,m,LabelCount,nLabelMin,nCol,nMaxLabel=10000;

    LabelCount = 2;
    pOut[0] = (pIn[0]) ? LabelCount++ : 0;
    for(j=1;j<nH;j++) {
        if(pIn[j]) {
            pOut[j] = (pOut[j-1]>1) ? pOut[j-1] : LabelCount++;
            LabelCount = (LabelCount>nMaxLabel) ? nMaxLabel : LabelCount;
        }
        else {
            pOut[j] = 0;
        }
    }
    ptsrc = pIn+nH;
    ptdst = pOut+nH;
    for(i=1;i<nV;i++) {
        if(ptsrc[0]) {
            ptdst[0] = (ptdst[-nH]>1) ? ptdst[-nH] : LabelCount++;
            LabelCount = (LabelCount>nMaxLabel) ? nMaxLabel : LabelCount;
        }
        else {
            ptdst[0] = 0;
        }
    }
    ptsrc = pIn+nH;
    ptdst = pOut+nH;
    for(i=1;i<nV;i++) {
        for(j=1;j<nH-1;j++) {
            if(ptsrc[j]) {
                nBuf[0] = ptdst[j-1];
                nBuf[1] = ptdst[j-1-nH];
                nBuf[2] = ptdst[j-nH];
                nBuf[3] = ptdst[j+1-nH];
                nLabelMin = nMaxLabel;
                for(k=0;k<4;k++) {
                    if(nBuf[k]<nLabelMin && nBuf[k]>0) {
                        nLabelMin = nBuf[k];
                    }
                }
                if(nLabelMin == nMaxLabel) {
                    ptdst[j] = LabelCount++;
                    LabelCount = (LabelCount>nMaxLabel) ? nMaxLabel : LabelCount;
                    continue;
                }
                else {
                    ptdst[j] = nLabelMin;
                    for(k=0;k<4;k++) {
                        if(nBuf[k]>1 && nBuf[k]!=nLabelMin) {
                            for(m=0;m<j;m++) {
                                if(ptdst[m] == nBuf[k]) {
                                    ptdst[m] = nLabelMin;
                                }
                            }
                            bCheck = true;
                            nCol = i;
                            ptdst1 = ptdst-nH;
                            while(bCheck) {
                                nCol--;
                                if(nCol < 0) {
                                    bCheck = false;
                                    continue;
                                }
                                bCheck = false;
                                for(m=0;m<nH;m++) {
                                    if(ptdst1[m] == nBuf[k]) {
                                        ptdst1[m] = nLabelMin;
                                        bCheck = true;
                                    }
                                }
                                ptdst1 -= nH;
                            }
                        }
                    }
                }
            }
            else {
                ptdst[j] = 0;
            }
        }
        ptsrc += nH;
        ptdst += nH;
    }
    m_nLabelPixelNo = 0;
    ptdst = pOut;
    for(i=0;i<nV;i++) {
        for(j=0;j<nH-1;j++) {
            if(ptdst[j] > 0) {
                m_ppLabelRec[m_nLabelPixelNo][0] = ptdst[j];
                m_ppLabelRec[m_nLabelPixelNo][1] = i;
                m_ppLabelRec[m_nLabelPixelNo][2] = j;
                m_nLabelPixelNo++;
            }
        }
        ptdst += nH;
    }
}

unsigned long MyGetTickCount(void)
{
    unsigned long currentTime;
#ifdef WIN32
    currentTime = GetTickCount();
#endif
    //#ifdef LINUX
    struct timeval current;
    gettimeofday(&current, NULL);
    currentTime = current.tv_sec * 1000 + current.tv_usec/1000;
    //#endif
#ifdef OS_VXWORKS
    ULONGA timeSecond = tickGet() / sysClkRateGet();
    ULONGA timeMilsec = tickGet() % sysClkRateGet() * 1000 / sysClkRateGet();
    currentTime = timeSecond * 1000 + timeMilsec;
#endif
    return currentTime;
}

unsigned long CAdvMotionTrack::IsRunMotionDetection(void)
{
    unsigned long dTick = MyGetTickCount();
    if(myabs(dTick-m_dTickRec) < (int)m_dTickInterval) {
        return 0;
    }
    return dTick;
}

bool CAdvMotionTrack::MotionDetection(unsigned char *pData,int nWidth,int nHeight,bool bYUY2)
{
    unsigned char *ptsrc;
    unsigned long dTick = MyGetTickCount();
    int i,j,x,y,nLabel,nT,nB,nL,nR;
    int nH,nV;
    int m_nHScaleXm_nVScale = m_nHScale * m_nVScale;

    dTick = IsRunMotionDetection();
    if (dTick <= 0) {
        return m_bHasValidObject;
    }
    m_dTickRec = dTick;
    if(bYUY2) {
        ScaledownYUY2(pData,m_pYDataScale,m_nH,m_nV,m_nScaleLevel);
    }
    else {
        ScaledownY(pData,m_pYDataScale,m_nH,m_nV,m_nScaleLevel);
    }

    register unsigned long vv;
    for(i = 0; i < (m_nHScaleXm_nVScale) >> 2; i++) {
        vv = *((unsigned long *)m_pMask +i);
        if(vv == 0x01010101) {
            continue;
        }
        else if(vv == 0x00000000) {
            *((unsigned long *)m_pYDataScale +i) = 0;
            continue;
        }

        m_pYDataScale[i*4 +0] *= GET_NTH_BYTE(vv, 0);
        m_pYDataScale[i*4 +1] *= GET_NTH_BYTE(vv, 1);
        m_pYDataScale[i*4 +2] *= GET_NTH_BYTE(vv, 2);
        m_pYDataScale[i*4 +3] *= GET_NTH_BYTE(vv, 3);
    }

    if(m_bDenoise == true) {
        for(i=0;i<1;i++) {
            DoHorizBlur(m_pYDataScale, m_nHScale, m_nVScale, m_pIdiff2, 1);
            DoVerticalBlur(m_pIdiff2, m_nHScale, m_nVScale, m_pYDataScale, 1);
        }
    }

    m_bHasValidObject = false;
    if(m_bFirstVideo == true) {
        memcpy(m_pIdiff, m_pYDataScale, m_nHScaleXm_nVScale);
        m_bFirstVideo = false;
    }
    else {

        for(i = 0; i < (m_nHScaleXm_nVScale >> 2); i++) {
            register unsigned long v1, v2, v3;
            v3 = 0;
            v1 = *((unsigned long *)m_pYDataScale +i);
            v2 = *((unsigned long *)m_pIdiff + i);
#if (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
            v3 |= ((myabs( GET_NTH_BYTE(v1, 0) - GET_NTH_BYTE(v2, 0) ) > m_nTd) ? 1 : 0);
            v3 |= ((myabs( GET_NTH_BYTE(v1, 1) - GET_NTH_BYTE(v2, 1) ) > m_nTd) ? 1 : 0) << 8;
            v3 |= ((myabs( GET_NTH_BYTE(v1, 2) - GET_NTH_BYTE(v2, 2) ) > m_nTd) ? 1 : 0) << 16;
            v3 |= ((myabs( GET_NTH_BYTE(v1, 3) - GET_NTH_BYTE(v2, 3) ) > m_nTd) ? 1 : 0) << 24;
#else
            v3 |= ((myabs( GET_NTH_BYTE(v1, 0) - GET_NTH_BYTE(v2, 0) ) > m_nTd) ? 1 : 0) << 24;
            v3 |= ((myabs( GET_NTH_BYTE(v1, 1) - GET_NTH_BYTE(v2, 1) ) > m_nTd) ? 1 : 0) << 16;
            v3 |= ((myabs( GET_NTH_BYTE(v1, 2) - GET_NTH_BYTE(v2, 2) ) > m_nTd) ? 1 : 0) << 8;
            v3 |= ((myabs( GET_NTH_BYTE(v1, 3) - GET_NTH_BYTE(v2, 3) ) > m_nTd) ? 1 : 0);
#endif

            *((unsigned long *)m_pIdiff1 + i) = v3;
        }

        if(myabs(dTick-m_dTickRecRef) > 1000) {
            m_dTickRecRef = dTick;
            memcpy(m_pIdiff,m_pYDataScale,m_nHScaleXm_nVScale);
        }
        ObjectLabeling(m_pIdiff1,m_pLabel,m_nHScale,m_nVScale);
        for(i=0;i<m_nLabelPixelNo;i++) {
            if(m_ppLabelRec[i][0] > 0) {
                nLabel = m_ppLabelRec[i][0];
                nL=m_nHScale; nT=m_nVScale; nR=nB=0;
                for(j=i;j<m_nLabelPixelNo;j++) {
                    if(m_ppLabelRec[j][0] == nLabel) {
                        nT = (m_ppLabelRec[j][1]<nT) ? m_ppLabelRec[j][1] : nT;
                        nB = (m_ppLabelRec[j][1]>nB) ? m_ppLabelRec[j][1] : nB;
                        nL = (m_ppLabelRec[j][2]<nL) ? m_ppLabelRec[j][2] : nL;
                        nR = (m_ppLabelRec[j][2]>nR) ? m_ppLabelRec[j][2] : nR;
                        m_ppLabelRec[j][0] = 0;
                    }
                }
                nH = nR-nL;
                nV = nB-nT;
                if(nH>m_nValidObjectSize[9] || nV>m_nValidObjectSize[9]) {
                    ptsrc = m_pSensitivity+nT*m_nHScale;
                    for(y=nT;y<nB;y++) {
                        for(x=nL;x<nR;x++) {
                            if(nH>m_nValidObjectSize[ptsrc[x]] || nV>m_nValidObjectSize[ptsrc[x]]) {
                                m_bHasValidObject = true;
                                y = nB;
                                i = m_nLabelPixelNo;
                                break;
                            }
                        }
                        ptsrc += m_nHScale;
                    }
                }
            }
        }
    }
    return m_bHasValidObject;
}
