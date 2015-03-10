// SaveConfig.cpp : 实现文件
//

#include "stdafx.h"
#include "USBCopy.h"
#include "SaveConfig.h"
#include "afxdialogex.h"
#include "Utils.h"

// CSaveConfig 对话框

IMPLEMENT_DYNAMIC(CSaveConfig, CDialogEx)

CSaveConfig::CSaveConfig(CWnd* pParent /*=NULL*/)
	: CDialogEx(CSaveConfig::IDD, pParent)
	, m_bCheckOverwrite(FALSE)
	, m_strEditConfigName(_T(""))
{

}

CSaveConfig::~CSaveConfig()
{
}

void CSaveConfig::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Check(pDX, IDC_CHECK_OVERWRITE, m_bCheckOverwrite);
	DDX_Text(pDX, IDC_EDIT_CONFIG_NAME, m_strEditConfigName);
}


BEGIN_MESSAGE_MAP(CSaveConfig, CDialogEx)
	ON_BN_CLICKED(IDOK, &CSaveConfig::OnBnClickedOk)
END_MESSAGE_MAP()


// CSaveConfig 消息处理程序


void CSaveConfig::OnBnClickedOk()
{
	// TODO: 在此添加控件通知处理程序代码
	UpdateData(TRUE);

	m_strEditConfigName.Trim();
	if (m_strEditConfigName.IsEmpty())
	{
		return;
	}

	if (m_strEditConfigName.Right(4).CompareNoCase(_T(".ini")) != 0)
	{
		m_strEditConfigName += _T(".ini");
	}

	CString strNewPath;
	strNewPath.Format(_T("%s\\%s"),m_strDestPath,m_strEditConfigName);

	if (!CopyFile(m_strSourcePath,strNewPath,!m_bCheckOverwrite))
	{
		AfxMessageBox(CUtils::GetErrorMsg(GetLastError()));

		m_strEditConfigName.Empty();

		UpdateData(FALSE);

		return;
	}

	CDialogEx::OnOK();
}

void CSaveConfig::SetSourcePath( CString strSourcePath )
{
	m_strSourcePath = strSourcePath;
}

void CSaveConfig::SetDestPath( CString strDestPath )
{
	m_strDestPath = strDestPath;
}
