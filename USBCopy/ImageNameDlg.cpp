// ImageNameDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "USBCopy.h"
#include "ImageNameDlg.h"
#include "afxdialogex.h"
#include "GlobalDef.h"
#include "Utils.h"


// CImageNameDlg 对话框

IMPLEMENT_DYNAMIC(CImageNameDlg, CDialogEx)

CImageNameDlg::CImageNameDlg(BOOL bImageMake,CWnd* pParent /*=NULL*/)
	: CDialogEx(CImageNameDlg::IDD, pParent)
	, m_strEditImageName(_T(""))
{
	m_pIni = NULL;
	m_bImageMake = bImageMake;
	m_ClientSocket = INVALID_SOCKET;
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
	ASSERT(m_pIni && m_ClientSocket != INVALID_SOCKET);

	m_strEditImageName = m_pIni->GetString(_T("ImagePath"),_T("ImageName"));

	if (m_bImageMake)
	{
		m_bServerFirst = m_pIni->GetBool(_T("ImageMake"),_T("SavePath"),FALSE);
	}
	else
	{
		m_bServerFirst = m_pIni->GetBool(_T("ImageCopy"),_T("PathType"),FALSE);
	}

	UpdateData(FALSE);
	
	return TRUE;  // return TRUE unless you set the focus to a control
	// 异常: OCX 属性页应返回 FALSE
}

void CImageNameDlg::SetConfig( CIni *pIni ,SOCKET sClient)
{
	m_pIni = pIni;
	m_ClientSocket = sClient;
}


void CImageNameDlg::OnBnClickedOk()
{
	// TODO: 在此添加控件通知处理程序代码
	UpdateData(TRUE);

	CString strResText;

	m_strEditImageName.Trim();
	if (m_strEditImageName.IsEmpty())
	{
		strResText.LoadString(IDS_MSG_EMPTY_IMAGE_NAME);
		MessageBox(strResText);
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
	BOOL bExist = FALSE;

	if (m_bImageMake)
	{
		// 制作到本地，还是服务器
		if (m_bServerFirst)
		{
			DWORD dwErrorCode = 0;
			bExist = QueryImage(m_strEditImageName,&dwErrorCode);

			if (dwErrorCode != 0)
			{
				MessageBox(CUtils::GetErrorMsg(dwErrorCode));

				return;
			}

		}
		else
		{
			bExist = PathFileExists(strImageFile);
		}

		// 文件是否存在，存在报错
		if (bExist)
		{
			strResText.LoadString(IDS_MSG_IMAGE_FILE_EXISTS);
			MessageBox(strResText);
			return;
		}
	}
	else
	{
		// 服务器优先，还是本地优先
		if (m_bServerFirst)
		{
			DWORD dwErrorCode = 0;
			bExist = QueryImage(m_strEditImageName,&dwErrorCode);

			if (dwErrorCode != 0)
			{
				MessageBox(CUtils::GetErrorMsg(dwErrorCode));
			}

			if (!bExist)
			{
				bExist = PathFileExists(strImageFile);

				if (bExist)
				{
					m_bServerFirst = FALSE;
				}
			}
			else
			{
				m_bServerFirst = TRUE;
			}
		}
		else
		{
			bExist = PathFileExists(strImageFile);

			if (!bExist)
			{
				DWORD dwErrorCode = 0;
				bExist = QueryImage(m_strEditImageName,&dwErrorCode);

				if (dwErrorCode != 0)
				{
					MessageBox(CUtils::GetErrorMsg(dwErrorCode));

					return;
				}

				if (bExist)
				{
					m_bServerFirst = TRUE;
				}
			}
			else
			{
				m_bServerFirst = FALSE;
			}
		}

		// 文件是否存在，不存在报错
		if (!bExist)
		{
			strResText.LoadString(IDS_MSG_IMAGE_FILE_NOT_EXISTS);
			MessageBox(strResText);
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

BOOL CImageNameDlg::QueryImage(CString strImageName,PDWORD pdwErrorCode)
{
	USES_CONVERSION;
	char *fileName = W2A(strImageName);

	DWORD dwLen = sizeof(CMD_IN) + strlen(fileName) + 1;

	char *bufSend = new char[dwLen];
	ZeroMemory(bufSend,dwLen);

	CMD_IN queryImageIn = {0};
	queryImageIn.dwCmdIn = CMD_QUERY_IMAGE_IN;
	queryImageIn.dwSizeSend = dwLen;

	memcpy(bufSend,&queryImageIn,sizeof(CMD_IN));
	memcpy(bufSend + sizeof(CMD_IN),fileName,strlen(fileName));

	if (!Send(m_ClientSocket,bufSend,dwLen,NULL,pdwErrorCode))
	{
		delete []bufSend;
		return FALSE;
	}

	delete []bufSend;

	QUERY_IMAGE_OUT queryImageOut = {0};
	dwLen = sizeof(QUERY_IMAGE_OUT);
	if (!Recv(m_ClientSocket,(char *)&queryImageOut,dwLen,NULL,pdwErrorCode))
	{
		return FALSE;
	}

	if (queryImageOut.dwCmdOut == CMD_QUERY_IMAGE_OUT && dwLen == sizeof(QUERY_IMAGE_OUT))
	{
		return (queryImageOut.dwErrorCode == 0);
	}

	return FALSE;
}

BOOL CImageNameDlg::GetServerFirst()
{
	return m_bServerFirst;
}
