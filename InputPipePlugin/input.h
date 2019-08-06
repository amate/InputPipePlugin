//----------------------------------------------------------------------------------
//	���̓v���O�C�� �w�b�_�[�t�@�C�� for AviUtl version 0.99k �ȍ~
//	By �j�d�m����
//----------------------------------------------------------------------------------

#pragma once

#include <Windows.h>
#include <Mmreg.h>

//	���̓t�@�C�����\����
typedef struct {
	int					flag;				//	�t���O
											//	INPUT_INFO_FLAG_VIDEO	: �摜�f�[�^����
											//	INPUT_INFO_FLAG_AUDIO	: �����f�[�^����
											//	INPUT_INFO_FLAG_VIDEO_RANDOM_ACCESS	: �L�[�t���[�����C�ɂ�����func_read_video()���Ăяo���܂�
											//	���W���ł̓L�[�t���[������V�[�P���V������func_read_video()���Ă΂��悤�ɐ��䂳��܂�
	int					rate,scale;			//	�t���[�����[�g
	int					n;					//	�t���[����
	BITMAPINFOHEADER	*format;			//	�摜�t�H�[�}�b�g�ւ̃|�C���^(���Ɋ֐����Ă΂��܂œ��e��L���ɂ��Ă���)
	int					format_size;		//	�摜�t�H�[�}�b�g�̃T�C�Y
	int					audio_n;			//	�����T���v����
	WAVEFORMATEX		*audio_format;		//	�����t�H�[�}�b�g�ւ̃|�C���^(���Ɋ֐����Ă΂��܂œ��e��L���ɂ��Ă���)
	int					audio_format_size;	//	�����t�H�[�}�b�g�̃T�C�Y
	DWORD				handler;			//	�摜codec�n���h��
	int					reserve[7];
} INPUT_INFO;
#define	INPUT_INFO_FLAG_VIDEO				1
#define	INPUT_INFO_FLAG_AUDIO				2
#define	INPUT_INFO_FLAG_VIDEO_RANDOM_ACCESS	8
//	���摜�t�H�[�}�b�g�ɂ�RGB,YUY2�ƃC���X�g�[������Ă���codec�̂��̂��g���܂��B
//	�܂��A'Y''C''4''8'(biBitCount��48)��PIXEL_YC�`���t�H�[�}�b�g�ň����܂��B(YUY2�t�B���^���[�h�ł͎g�p�o���܂���)
//	�����t�H�[�}�b�g�ɂ�PCM�ƃC���X�g�[������Ă���codec�̂��̂��g���܂��B

//	���̓t�@�C���n���h��
typedef void*	INPUT_HANDLE;

//	���̓v���O�C���\����
typedef struct {
	int		flag;				//	�t���O
								//	INPUT_PLUGIN_FLAG_VIDEO	: �摜���T�|�[�g����
								//	INPUT_PLUGIN_FLAG_AUDIO	: �������T�|�[�g����
	LPCSTR	name;				//	�v���O�C���̖��O
	LPCSTR	filefilter;			//	���̓t�@�C���t�B���^
	LPCSTR	information;		//	�v���O�C���̏��
	BOOL 	(*func_init)( void );
								//	DLL�J�n���ɌĂ΂��֐��ւ̃|�C���^ (NULL�Ȃ�Ă΂�܂���)
	BOOL 	(*func_exit)( void );
								//	DLL�I�����ɌĂ΂��֐��ւ̃|�C���^ (NULL�Ȃ�Ă΂�܂���)
	INPUT_HANDLE (*func_open)( LPSTR file );
								//	���̓t�@�C�����I�[�v������֐��ւ̃|�C���^
								//	file	: �t�@�C����
								//	�߂�l	: TRUE�Ȃ���̓t�@�C���n���h��
	BOOL 	(*func_close)( INPUT_HANDLE ih );
								//	���̓t�@�C�����N���[�Y����֐��ւ̃|�C���^
								//	ih		: ���̓t�@�C���n���h��
								//	�߂�l	: TRUE�Ȃ琬��
	BOOL 	(*func_info_get)( INPUT_HANDLE ih,INPUT_INFO *iip );
								//	���̓t�@�C���̏����擾����֐��ւ̃|�C���^
								//	ih		: ���̓t�@�C���n���h��
								//	iip		: ���̓t�@�C�����\���̂ւ̃|�C���^
								//	�߂�l	: TRUE�Ȃ琬��
	int 	(*func_read_video)( INPUT_HANDLE ih,int frame,void *buf );
								//	�摜�f�[�^��ǂݍ��ފ֐��ւ̃|�C���^
								//	ih		: ���̓t�@�C���n���h��
								//	frame	: �ǂݍ��ރt���[���ԍ�
								//	buf		: �f�[�^��ǂݍ��ރo�b�t�@�ւ̃|�C���^
								//	�߂�l	: �ǂݍ��񂾃f�[�^�T�C�Y
	int 	(*func_read_audio)( INPUT_HANDLE ih,int start,int length,void *buf );
								//	�����f�[�^��ǂݍ��ފ֐��ւ̃|�C���^
								//	ih		: ���̓t�@�C���n���h��
								//	start	: �ǂݍ��݊J�n�T���v���ԍ�
								//	length	: �ǂݍ��ރT���v����
								//	buf		: �f�[�^��ǂݍ��ރo�b�t�@�ւ̃|�C���^
								//	�߂�l	: �ǂݍ��񂾃T���v����
	BOOL 	(*func_is_keyframe)( INPUT_HANDLE ih,int frame );
								//	�L�[�t���[�������ׂ�֐��ւ̃|�C���^ (NULL�Ȃ�S�ăL�[�t���[��)
								//	ih		: ���̓t�@�C���n���h��
								//	frame	: �t���[���ԍ�
								//	�߂�l	: �L�[�t���[���Ȃ琬��
	BOOL	(*func_config)( HWND hwnd,HINSTANCE dll_hinst );
								//	���͐ݒ�̃_�C�A���O��v�����ꂽ���ɌĂ΂��֐��ւ̃|�C���^ (NULL�Ȃ�Ă΂�܂���)
								//	hwnd		: �E�B���h�E�n���h��
								//	dll_hinst	: �C���X�^���X�n���h��
								//	�߂�l		: TRUE�Ȃ琬��
	int		reserve[16];
} INPUT_PLUGIN_TABLE;
#define	INPUT_PLUGIN_FLAG_VIDEO		1
#define	INPUT_PLUGIN_FLAG_AUDIO		2

BOOL func_init( void );
BOOL func_exit( void );
INPUT_HANDLE func_open( LPSTR file );
BOOL func_close( INPUT_HANDLE ih );
BOOL func_info_get( INPUT_HANDLE ih,INPUT_INFO *iip );
int func_read_video( INPUT_HANDLE ih,int frame,void *buf );
int func_read_audio( INPUT_HANDLE ih,int start,int length,void *buf );
BOOL func_is_keyframe( INPUT_HANDLE ih,int frame );
BOOL func_config( HWND hwnd,HINSTANCE dll_hinst );


