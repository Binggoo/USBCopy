// MyMessageBox.cpp : 实现文件
//

#include "stdafx.h"
#include "USBCopy.h"
#include "MyMessageBox.h"
#include "afxdialogex.h"


// CMyMessageBox 对话框

IMPLEMENT_DYNAMIC(CMyMessageBox, CDialogEx)

CMyMessageBox::CMyMessageBox(CWnd* pParent /*=NULL*/)
	: CDialogEx(CMyMessageBox::IDD, pParent)
{
	m_pBitmap = NULL;
	m_pFont = NULL;
	m_rgbTextColor = RGB(0,0,0);
}

CMyMessageBox::~CMyMessageBox()
{
	if (m_pBitmap)
	{
		m_pBitmap->DeleteObject();

		delete m_pBitmap;
	}

	if (m_pFont)
	{
		m_pFont->DeleteObject();

		delete m_pFont;
	}
}

void CMyMessageBox::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_BITMAP, m_PicBitmap);
}


BEGIN_MESSAGE_MAP(CMyMessageBox, CDialogEx)
	ON_WM_CTLCOLOR()
END_MESSAGE_MAP()


// CMyMessageBox 消息处理程序


BOOL CMyMessageBox::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// TODO:  在此添加额外的初始化
	if (m_pFont == NULL)
	{
		m_pFont = new CFont();
		m_pFont->CreatePointFont(120,_T("Arial"));
	}

	if (m_pBitmap == NULL)
	{
		m_pBitmap = new CBitmap();
		m_pBitmap->LoadBitmap(IDB_WARNING);
	}

	m_PicBitmap.SetBitmap((HBITMAP)m_pBitmap->m_hObject);

	GetDlgItem(IDC_TEXT_MSG)->SetFont(m_pFont);

	SetDlgItemText(IDC_TEXT_MSG,m_strText);
	SetWindowText(m_strCaption);

	return TRUE;  // return TRUE unless you set the focus to a control
	// 异常: OCX 属性页应返回 FALSE
}

void CMyMessageBox::SetFont( CFont* pFont )
{
	m_pFont = pFont;
}

void CMyMessageBox::SetFont( UINT nSize )
{
	if (m_pFont == NULL)
	{
		m_pFont = new CFont();
	}

	m_pFont->CreatePointFont(nSize,_T("Arial"));
}

void CMyMessageBox::SetBitmap( CBitmap *pBitmap )
{
	m_pBitmap = pBitmap;
}

void CMyMessageBox::SetBitmap( UINT nID )
{
	if (m_pBitmap == NULL)
	{
		m_pBitmap = new CBitmap();
	}

	if (m_pBitmap->m_hObject != NULL)
	{
		m_pBitmap->DeleteObject();
		m_pBitmap->m_hObject = NULL;
	}

	m_pBitmap->LoadBitmap(nID);
}

void CMyMessageBox::SetTitle( LPCTSTR lpcszCaption )
{
	m_strCaption = CString(lpcszCaption);
}

void CMyMessageBox::SetTitle( UINT nID )
{
	m_strCaption.LoadString(nID);
}

void CMyMessageBox::SetText( LPCTSTR lpcszText )
{
	m_strText = CString(lpcszText);
}

void CMyMessageBox::SetText( UINT nID )
{
	m_strText.LoadString(nID);
}

void CMyMessageBox::SetTextColor( COLORREF color )
{
	m_rgbTextColor = color;
}


HBRUSH CMyMessageBox::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	HBRUSH hbr = CDialogEx::OnCtlColor(pDC, pWnd, nCtlColor);

	// TODO:  在此更改 DC 的任何特性
	if (pWnd->GetDlgCtrlID() == IDC_TEXT_MSG)
	{
		pDC->SetTextColor(m_rgbTextColor);
	}

	// TODO:  如果默认的不是所需画笔，则返回另一个画笔
	return hbr;
}
