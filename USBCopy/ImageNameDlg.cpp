// ImageNameDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "USBCopy.h"
#include "ImageNameDlg.h"
#include "afxdialogex.h"


// CImageNameDlg 对话框

IMPLEMENT_DYNAMIC(CImageNameDlg, CDialogEx)

CImageNameDlg::CImageNameDlg(BOOL bImageMake,CWnd* pParent /*=NULL*/)
	: CDialogEx(CImageNameDlg::IDD, pParent)
	, m_strEditImageName(_T(""))
{
	m_pIni = NULL;
	m_bImageMake = bImageMake;
}

CImageNameDlg::~CImageNameDlg()
{
}

void CImageNameDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_EDIT_IMAGE_NAME, m_strEditImageName);
}


BEGIN_MESSAGE_MAP(CImageNameDlg, CDialogEx)
	ON_BN_CLICKED(IDOK, &CImageNameDlg::OnBnClickedOk)
END_MESSAGE_MAP()


// CImageNameDlg 消息处理程序


BOOL CImageNameDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// TODO:  在此添加额外的初始化
	ASSERT(m_pIni);

	m_strEditImageName = m_pIni->GetString(_T("ImagePath"),_T("ImageName"));

	UpdateData(FALSE);
	
	return TRUE;  // return TRUE unless you set the focus to a control
	// 异常: OCX 属性页应返回 FALSE
}

void CImageNameDlg::SetConfig( CIni *pIni )
{
	m_pIni = pIni;
}


void CImageNameDlg::OnBnClickedOk()
{
	// TODO: 在此添加控件通知处理程序代码
	UpdateData(TRUE);
	m_strEditImageName.Trim();
	if (m_strEditImageName.IsEmpty())
	{
		MessageBox(_T("The image name can't be empty, please input the image name."),_T("Error"),MB_ICONERROR);
		return;
	}

	CString strImagePath = m_pIni->GetString(_T("ImagePath"),_T("ImagePath"),_T("d:\\image"));
	strImagePath.TrimRight(_T('\\'));
	if (m_strEditImageName.Right(4).CompareNoCase(_T(".IMG")) != 0)
	{
		m_strEditImageName += _T(".IMG");
	}

	CString strImageFile;
	strImageFile.Format(_T("%s\\%s"),strImagePath,m_strEditImageName);
	BOOL bExist = PathFileExists(strImageFile);

	if (m_bImageMake)
	{
		// 文件是否存在，存在报错
		if (bExist)
		{
			MessageBox(_T("The image file already exists."));
			return;
		}
	}
	else
	{
		if (!bExist)
		{
			MessageBox(_T("The image file doesn'texists."));
			return;
		}
	}

	m_pIni->WriteString(_T("ImagePath"),_T("ImageName"),m_strEditImageName);
	CDialogEx::OnOK();
}

void CImageNameDlg::SetMakeImage( BOOL bImageMake /*= TRUE*/ )
{
	m_bImageMake = bImageMake;
}
