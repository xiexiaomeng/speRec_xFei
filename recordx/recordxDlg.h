
// recordxDlg.h : ͷ�ļ�
//
#include<mmsystem.h>
#include "afxwin.h"
//#define INP_BUFFER_SIZE 16*1024
#define INP_BUFFER_SIZE 16*1024
#pragma once


// CrecordxDlg �Ի���
class CrecordxDlg : public CDialogEx
{
// ����
public:
	CrecordxDlg(CWnd* pParent = NULL);	// ��׼���캯��

	////////���ݳ�Ա//////////
	DWORD m_dwDataLength;        //���ݳ���
	WAVEFORMATEX m_waveform;    //������ʽ
	HWAVEIN m_hWaveIn;            //��Ƶ������
	HWAVEOUT m_hWaveOut;        //��Ƶ������
	PBYTE m_pSaveBuffer;        //��Ƶ�洢�ڴ�

	WAVEHDR m_WAVEHDR1;
	WAVEHDR m_WAVEHDR2;

	enum{ RECORD_STATE_INIT,
		RECORD_STATE_RECING,
		RECORD_STATE_STOP,
	    RECORD_STATE_STOPING,
	    RECORD_STATE_PLAYING} 
	m_nRecordState;

	char m_cbBuffer1[INP_BUFFER_SIZE];    //������ʱ����1
	char m_cbBuffer2[INP_BUFFER_SIZE];    //������ʱ����2

// �Ի�������
	enum { IDD = IDD_RECORDX_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV ֧��


// ʵ��
protected:
	HICON m_hIcon;

	// ���ɵ���Ϣӳ�亯��
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();

	afx_msg void OnBnClickedButton1();	
	afx_msg void OnBnClickedButton2();
	afx_msg void OnBnClickedButton3();
	afx_msg LRESULT OnMM_WIM_OPEN(UINT wParam,LONG lParam);
	afx_msg LRESULT OnMM_WIM_DATA(UINT wParam,LONG lParam);
	afx_msg LRESULT OnMM_WIM_CLOSE(UINT wParam,LONG lParam);
	afx_msg LRESULT OnMM_WON_OPEN(UINT wParam,LONG lParam);
	afx_msg LRESULT OnMM_WOM_DONE(UINT wParam,LONG lParam);
	afx_msg LRESULT OnMM_WOM_CLOSE(UINT wParam,LONG lParam);	

	DECLARE_MESSAGE_MAP()
public:

	afx_msg void OnBnClickedButton4();
	afx_msg void OnBnClickedButton5();
	afx_msg void OnEnChangeEdit1();
	//CEdit m_editMultiLine;
	afx_msg void OnBnClickedButton6();
	afx_msg void OnBnClickedButton7();
};
