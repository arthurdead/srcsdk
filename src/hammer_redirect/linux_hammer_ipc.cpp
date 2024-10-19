#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN

#define NOGDICAPMASKS
#define NOVIRTUALKEYCODES
//#define NOWINMESSAGES
#define NOWINSTYLES
#define NOSYSMETRICS
#define NOMENUS
#define NOICONS
#define NOKEYSTATES
#define NOSYSCOMMANDS
#define NORASTEROPS
#define NOSHOWWINDOW
#define OEMRESOURCE
#define NOATOM
#define NOCLIPBOARD
#define NOCOLOR
#define NOCTLMGR
#define NODRAWTEXT
#define NOGDI
#define NOKERNEL
#define NOUSER
#define NONLS
#define NOMB
#define NOMEMMGR
#define NOMETAFILE
#define NOMINMAX
#define NOMSG
#define NOOPENFILE
#define NOSCROLL
#define NOSERVICE
#define NOSOUND
#define NOTEXTMETRIC
#define NOWH
#define NOWINOFFSETS
#define NOCOMM
#define NOKANJI
#define NOHELP
#define NOPROFILER
#define NODEFERWINDOWPOS
#define NOMCX

#include <windows.h>

#include <cstdlib>

#include <unistd.h>
#include <fcntl.h>

#include "linux_hammer_ipc.h"

static HWND hammer_wnd = nullptr;
static char hammer_cmd_buffer[2048];

#define HAMMER_WINDOW_CLASSNAME "Worldcraft_ShellMessageWnd"
#define HAMMER_WINDOW_NAME "Worldcraft_ShellMessageWnd"

int main(int argc, char *argv[])
{
	unsigned char ret = LINUX_HAMMER_IPC_RET_FAIL;

	for(;;) {
		unsigned char code;
		if(read(STDIN_FILENO, &code, 1) == 1) {
			switch(code) {
			case LINUX_HAMMER_IPC_QUIT:
				ret = LINUX_HAMMER_IPC_RET_SUCCESS;
				write(STDOUT_FILENO, &ret, sizeof(unsigned char));
				return EXIT_SUCCESS;
			case LINUX_HAMMER_IPC_FIND_WND:
				hammer_wnd = FindWindowA(HAMMER_WINDOW_CLASSNAME, HAMMER_WINDOW_NAME);
				if(hammer_wnd) {
					ret = LINUX_HAMMER_IPC_RET_SUCCESS;
				} else {
					ret = LINUX_HAMMER_IPC_RET_NOT_RUNNING;
				}
				write(STDOUT_FILENO, &ret, sizeof(unsigned char));
				break;
			case LINUX_HAMMER_IPC_SEND_CMD:
				if(!hammer_wnd) {
					hammer_wnd = FindWindowA(HAMMER_WINDOW_CLASSNAME, HAMMER_WINDOW_NAME);
				}
				if(hammer_wnd) {
					read(STDIN_FILENO, hammer_cmd_buffer, sizeof(hammer_cmd_buffer));

					COPYDATASTRUCT CopyData;
					CopyData.cbData = strlen(hammer_cmd_buffer) + 1;
					CopyData.dwData = 0;
					CopyData.lpData = (void *)hammer_cmd_buffer;

					if(SendMessageA(hammer_wnd, WM_COPYDATA, 0, (LPARAM)&CopyData)) {
						ret = LINUX_HAMMER_IPC_RET_SUCCESS;
					} else {
						ret = LINUX_HAMMER_IPC_RET_FAIL;
					}
				} else {
					ret = LINUX_HAMMER_IPC_RET_NOT_RUNNING;
				}
				write(STDOUT_FILENO, &ret, sizeof(unsigned char));
				break;
			default:
				ret = LINUX_HAMMER_IPC_RET_UNKNOWN_CODE;
				write(STDOUT_FILENO, &ret, sizeof(unsigned char));
				break;
			}
		}
	}

	return EXIT_SUCCESS;
}
