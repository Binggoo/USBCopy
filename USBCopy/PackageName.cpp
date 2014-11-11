// PackageName.cpp : 实现文件
//

#include "stdafx.h"
#include "USBCopy.h"
#include "PackageName.h"
#include "Utils.h"
#include "afxdialogex.h"


// CPackageName 对话框

IMPLEMENT_DYNAMIC(CPackageName, CDialogEx)

CPackageName::CPackageName(CWnd* pParent /*=NULL*/)
	: CDialogEx(CPackageName::IDD, pParent)
{
	m_pIni = NULL;
	m_ClientSocket = INVALID_SOCKET;
	m_MasterPort = NULL;
}

CPackageName::~CPackageName()
{
}

void CPackageName::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Text(pDX,IDC_EDIT_PKG_NAME,m_strEditPackageName);
}


BEGIN_MESSAGE_MAP(CPackageName, CDialogEx)
	ON_BN_CLICKED(IDOK, &CPackageName::OnBnClickedOk)
END_MESSAGE_MAP()


// CPackageName 消息处理程序


BOOL CPackageName::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// TODO:  在此添加额外的初始化
	ASSERT(m_pIni);

	m_strEditPackageName = m_pIni->GetString(_T("DifferenceCopy"),_T("PkgName"));

	UpdateData(FALSE);

	return TRUE;  // return TRUE unless you set the focus to a control
	// 异常: OCX 属性页应返回 FALSE
}

void CPackageName::SetConfig( CIni *pIni,CPort *port,SOCKET sClient )
{
	m_pIni = pIni;
	m_ClientSocket = sClient;
	m_MasterPort = port;
}


void CPackageName::OnBnClickedOk()
{
	// TODO: 在此添加控件通知处理程序代码

	UpdateData(TRUE);

	CString strResText;

	m_strEditPackageName.Trim();
	if (m_strEditPackageName.IsEmpty())
	{
		strResText.LoadString(IDS_MSG_EMPTY_IMAGE_NAME);
		MessageBox(strResText);
		return;
	}

	BOOL bRemote = m_pIni->GetBool(_T("DifferenceCopy"),_T("PkgLocation"),0); //1 - Remote, 0 - Local

	// 查询增量包是否存在
	if (!bRemote)
	{
		CString strResText;
		if (!m_MasterPort->IsConnected())
		{
			strResText.LoadString(IDS_MSG_NO_MASTER);
			MessageBox(strResText);

			return;
		}
		else if(!PathFileExists(_T("M:\\")))
		{
			strResText.LoadString(IDS_MSG_MASTER_UNRECOGNIZED);
			MessageBox(strResText);

			return;
		}

		CString strPackage;
		strPackage.Format(_T("M:\\%s"),m_strEditPackageName);

		if (!PathFileExists(strPackage))
		{
			strResText.LoadString(IDS_MSG_PKG_FOLDER_NOT_EXIST);
			MessageBox(strResText);

			return;
			
		}
	}
	else
	{
		//服务器上查询
		CString strChangList;
		strChangList.Format(_T("%s.ini"),m_strEditPackageName);
		DWORD dwErrorCode = 0;

		BOOL bQuery = QueryPackage(m_strEditPackageName,&dwErrorCode);

		bQuery |= QueryChangeList(strChangList,&dwErrorCode);

		if (!bQuery)
		{
			if (dwErrorCode == 0)
			{
				strResText.LoadString(IDS_MSG_PKG_FOLDER_NOT_EXIST);
				MessageBox(strResText);
			}
			else
			{
				MessageBox(CUtils::GetErrorMsg(dwErrorCode));
			}

			return;
		}
		
	}

	m_pIni->WriteString(_T("DifferenceCopy"),_T("PkgName"),m_strEditPackageName);

	CDialogEx::OnOK();
}

BOOL CPackageName::QueryPackage( CString strPackageName,PDWORD pdwErrorCode )
{
	USES_CONVERSION;
	char *pkg = W2A(strPackageName);

	DWORD dwLen = sizeof(CMD_IN) + strlen(pkg) + 2;
	BYTE *sendBuf = new BYTE[dwLen];
	ZeroMemory(sendBuf,dwLen);
	sendBuf[dwLen - 1] = END_FLAG;

	CMD_IN cmdIn = {0};
	cmdIn.dwCmdIn = CMD_QUERY_PKG_IN;
	cmdIn.dwSizeSend = dwLen;

	memcpy(sendBuf,&cmdIn,sizeof(CMD_IN));
	memcpy(sendBuf + sizeof(CMD_IN),pkg,strlen(pkg));

	if (!Send(m_ClientSocket,(char *)sendBuf,dwLen,NULL,pdwErrorCode))
	{
		delete []sendBuf;

		return FALSE;
	}

	delete []sendBuf;

	QUERY_PKG_OUT queryPkgOut = {0};
	dwLen = sizeof(QUERY_IMAGE_OUT);
	if (!Recv(m_ClientSocket,(char *)&queryPkgOut,dwLen,NULL,pdwErrorCode))
	{
		return FALSE;
	}

	if (queryPkgOut.dwCmdOut == CMD_QUERY_PKG_OUT && dwLen == sizeof(QUERY_PKG_OUT))
	{
		if (queryPkgOut.dwErrorCode != 0)
		{
			return FALSE;
		}
		else
		{
			return TRUE;
		}
	}
	else
	{
		return FALSE;
	}
}


BOOL CPackageName::QueryChangeList( CString strChangeList,PDWORD pdwErrorCode )
{
	USES_CONVERSION;
	char *change = W2A(strChangeList);

	DWORD dwLen = sizeof(CMD_IN) + strlen(change) + 2;
	BYTE *sendBuf = new BYTE[dwLen];
	ZeroMemory(sendBuf,dwLen);
	sendBuf[dwLen - 1] = END_FLAG;

	CMD_IN cmdIn = {0};
	cmdIn.dwCmdIn = CMD_QUERY_LIST_IN;
	cmdIn.dwSizeSend = dwLen;

	memcpy(sendBuf,&cmdIn,sizeof(CMD_IN));
	memcpy(sendBuf + sizeof(CMD_IN),change,strlen(change));

	if (!Send(m_ClientSocket,(char *)sendBuf,dwLen,NULL,pdwErrorCode))
	{
		delete []sendBuf;

		return FALSE;
	}

	delete []sendBuf;

	QUERY_LIST_OUT queryListOut = {0};
	dwLen = sizeof(QUERY_LIST_OUT);
	if (!Recv(m_ClientSocket,(char *)&queryListOut,dwLen,NULL,pdwErrorCode))
	{
		return FALSE;
	}

	if (queryListOut.dwCmdOut == CMD_QUERY_LIST_OUT && dwLen == sizeof(QUERY_LIST_OUT))
	{
		if (queryListOut.dwErrorCode != 0)
		{
			return FALSE;
		}
		else
		{
			return TRUE;
		}
	}
	else
	{
		return FALSE;
	}
}
