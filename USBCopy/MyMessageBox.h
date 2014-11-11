#pragma once
#include "afxwin.h"


// CMyMessageBox �Ի���

class CMyMessageBox : public CDialogEx
{
	DECLARE_DYNAMIC(CMyMessageBox)

public:
	CMyMessageBox(CWnd* pParent = NULL);   // ��׼���캯��
	virtual ~CMyMessageBox();

// �Ի�������
	enum { IDD = IDD_DIALOG_MSGBOX };

	void SetFont(CFont* pFont);
	void SetFont(UINT nSize);
	void SetBitmap(CBitmap *pBitmap);
	void SetBitmap(UINT nID);
	void SetTitle(LPCTSTR lpcszCaption);
	void SetTitle(UINT nID);
	void SetText(LPCTSTR lpcszText);
	void SetText(UINT nID);
	void SetTextColor(COLORREF color);

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV ֧��

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
private:
	CStatic m_PicBitmap;
	CBitmap *m_pBitmap;
	CFont   *m_pFont;
	CString m_strCaption;
	CString m_strText;
	COLORREF m_rgbTextColor;
public:
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
};
