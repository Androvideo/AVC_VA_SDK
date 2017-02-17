#define _SCALEBITS			16
#define _FIX(X)				((1L << _SCALEBITS) / (X) + 1)

class CAdvMotionTrack
{
    public:
        CAdvMotionTrack(int nWidth,int nHeight,int nSensitivity,bool bDenoise, int is_USE_CIF_Detect);
        ~CAdvMotionTrack();
        void AddMotionArea(int nL,int nT,int nR,int nB,int nWidthBase,int nHeightBase,int nSensitivity);
        void DelMotionArea(int nL,int nT,int nR,int nB,int nWidthBase,int nHeightBase);
        bool MotionDetection(unsigned char *pData,int nWidth,int nHeight,bool bYUY2);
    private:
        unsigned long IsRunMotionDetection(void);
        void ScaledownYUY2(unsigned char *pIn,unsigned char *pOut,int nWidth,int nHeight,int nLevel);
        void ObjectLabeling(unsigned char *pIn,short *pOut,int nH,int nV);
        void DoHorizBlur(unsigned char *src, int src_w, int src_h, unsigned char *dst, int boxw);
        void DoVerticalBlur(unsigned char *src, int src_w, int src_h, unsigned char *dst, int boxw);
        void ScaledownY(unsigned char *pIn,unsigned char *pOut,int nWidth,int nHeight,int nLevel);
        bool m_bHasValidObject,m_bFirstVideo,m_bFirstMask,m_bDenoise;
        int m_nSensitivity,m_nValidObjectSize[10];
        short **m_ppLabelRec,*m_pLabel;
        unsigned char *m_pIdiff,*m_pIdiff1,*m_pIdiff2,*m_pYData,*m_pYDataScale,*m_pMask,*m_pSensitivity;
        int *m_pIntegral,*m_pnBuffer,m_nTd,m_nScaleLevel,m_nLabelPixelNo,m_nPointNo;
        int m_nH,m_nV,m_nHScale,m_nVScale;
        unsigned long m_dTickInterval,m_dTickRec,m_dTickRecRef;
};
