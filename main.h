#include <windows.h>
#include <vector>
#include <string>
#include <fstream>
#include <time.h>
#include <d3d9.h>
#pragma comment(lib, "d3d9.lib")

#include "DXSDK\d3dx9.h"
#if defined _M_X64
#pragma comment(lib, "DXSDK/x64/d3dx9.lib") 
#elif defined _M_IX86
#pragma comment(lib, "DXSDK/x86/d3dx9.lib")
#endif

//#include <d3dx9.h>
//#pragma comment(lib, "d3dx9.lib") 
#pragma comment(lib, "winmm.lib")
#include "MinHook/include/MinHook.h" //detour
using namespace std;

#pragma warning (disable: 4244) //

//==========================================================================================================================

HMODULE mpubHand;

//Stride
UINT Stride;

//elementcount
D3DVERTEXELEMENT9 decl[MAXD3DDECLLENGTH];
UINT numElements;

//vertexshaderconstantf
UINT mStartRegister;
UINT mVector4fCount;

//vdesc.Size
//D3DVERTEXBUFFER_DESC vdesc;

//vertexshader
IDirect3DVertexShader9* vShader;
UINT vSize;

//pixelshader
IDirect3DPixelShader9* pShader;
UINT pSize;

//texture crc
IDirect3DTexture9* pCurrentTex = NULL;
DWORD qlCRC;
int dWidth;
int dHeight;
//int dFormat;
int mStage;

D3DVIEWPORT9 Viewport; //use this viewport
float ScreenCX;
float ScreenCY;

//logger
//bool logger = false;
//int countnum = -1;

//features
int wallhack = 1;				//wallhack
int occlusion = 1;				//occlusion exploit

//aimbot settings
int aimbot = 1;
int aimkey = 2;
DWORD Daimkey = VK_RBUTTON;		//aimkey
int aimsens = 3;				//aim sensitivity, makes aim smoother
int aimfov = 6;					//aim field of view in % 
int aimheight = 2;				//aim height value for mensa, low value = aims heigher, high values aims lower
int aimheightxy = 1;			//real value, aimheight * 4 + 27
int esp = 2;					//anim pic esp

int useaimhuman = 0;			//human aim sensitivity value
int aim_pause = 5;
bool after_kill = false;
bool smooth_on = false;

bool sound = false;
int killsounds = 0;

//autoshoot settings
int autoshoot = 1;
unsigned int asdelay = 50;		//use x-999 (shoot for xx millisecs, looks more legit)
bool IsPressd = false;			//

//timer
DWORD astime = timeGetTime();	//auto_shoot
DWORD frameafterkill = timeGetTime(); //aim_pause
DWORD framesmooth = timeGetTime(); //aimpause
DWORD soundpause = timeGetTime(); //killsounds

//sprite timer
DWORD dwStartTime0 = 0; //time as the timer started
DWORD dwTime0 = 0; //windowsuptime
DWORD dwStartTime1 = 0; //time as the timer started
DWORD dwTime1 = 0; //windowsuptime
//==========================================================================================================================

// getdir & log
char mpubdir[320];
char* GetmpubDirFile(char *plname)
{
	static char pldir[320];
	strcpy_s(pldir, mpubdir);
	strcat_s(pldir, plname);
	return pldir;
}
/*
void Log(const char *fmt, ...)
{
	if (!fmt)	return;

	char		text[4096];
	va_list		ap;
	va_start(ap, fmt);
	vsprintf_s(text, fmt, ap);
	va_end(ap);

	ofstream logfile(GetmpubDirFile("log.txt"), ios::app);
	if (logfile.is_open() && text)	logfile << text << endl;
	logfile.close();
}
*/
DWORD QuickmpubChecksum(DWORD *pDatar, int size)
{
	if (!pDatar) { return 0x0; }

	DWORD sum;
	DWORD tmp;
	sum = *pDatar;

	for (int i = 1; i < (size / 4); i++)
	{
		tmp = pDatar[i];
		tmp = (DWORD)(sum >> 29) + tmp;
		tmp = (DWORD)(sum >> 17) + tmp;
		sum = (DWORD)(sum << 3) ^ tmp;
	}

	return sum;
}

//==========================================================================================================================

//get distance
float GetmDst(float Xx, float Yy, float xX, float yY)
{
	return sqrt((yY - Yy) * (yY - Yy) + (xX - Xx) * (xX - Xx));
}

struct AimHPBarInfo_t
{
	float vOutX, vOutY;
	INT       iTeam;
	float CrosshairDst;
};
std::vector<AimHPBarInfo_t>AimHPBarInfo;
//float RealDistance;

// Parameters:
//
//   float4 mvp[4];
//
//
// Registers:
//
//   Name         Reg   Size
//   ------------ ----- ----
//   mvp          c8       4
//w2s for health bars
void HPBarAim(LPDIRECT3DDEVICE9 Device, int iTeam)
{
	D3DXVECTOR3 from, to;
	D3DXMATRIX mvp, world;

	//not here
	//D3DVIEWPORT9 Viewport;
	//Device->GetViewport(&Viewport);

	Device->GetVertexShaderConstantF(8, mvp, 4);//8mvp
	//Device->GetVertexShaderConstantF(240, world, 4);//world 240-243, 247-252

	float w = 0.0f;
	to[0] = mvp[0] * world._14 + mvp[1] * world._24 + mvp[2] * world._34 + mvp[3];
	to[1] = mvp[4] * world._14 + mvp[5] * world._24 + mvp[6] * world._34 + mvp[7];
	w = mvp[12] * world._14 + mvp[13] * world._24 + mvp[14] * world._34 + mvp[15];

	if (w > 0.01f)
	{
		//aimheightxy = (Viewport.Height / 50) + (aimheight * 3);
		aimheightxy = ((float)Viewport.Height * 0.02f) + (aimheight * 3);

		float invw = 1.0f / w;
		to[0] *= invw;
		to[1] *= invw;

		float x = Viewport.Width / 2.0f;
		float y = Viewport.Height / 2.0f;

		x += 0.5f * to[0] * Viewport.Width + 0.5f;
		y -= 0.5f * to[1] * Viewport.Height - aimheightxy;
		to[0] = x + Viewport.X;
		to[1] = y + Viewport.Y;
	}
	else
	{
		to[0] = -1.0f;
		to[1] = -1.0f;
	}

	//to[0] = X
	//to[1] = Y

		//AimHPBarInfo_t pAimHPBarInfo = { static_cast<float>(to[0] + (Viewport.Width*0.5f)), static_cast<float>(to[1] + (Viewport.Height*0.5f)), iTeam };
		AimHPBarInfo_t pAimHPBarInfo = { static_cast<float>(to[0]), static_cast<float>(to[1]), iTeam };
		AimHPBarInfo.push_back(pAimHPBarInfo);
}

struct EspHPBarInfo_t
{
	float vOutX, vOutY;
	INT       iTeam;
	float CrosshairDst;
};
std::vector<EspHPBarInfo_t>EspHPBarInfo;
//float RealDistance;

// Parameters:
//
//   float4 mvp[4];
//
//
// Registers:
//
//   Name         Reg   Size
//   ------------ ----- ----
//   mvp          c8       4
//w2s for health bars
void HPBarEsp(LPDIRECT3DDEVICE9 Device, int iTeam)
{
	D3DXVECTOR3 from, to;
	D3DXMATRIX mvp, world;

	//not here
	//D3DVIEWPORT9 Viewport;
	//Device->GetViewport(&Viewport);

	Device->GetVertexShaderConstantF(8, mvp, 4);//8mvp
	Device->GetVertexShaderConstantF(240, world, 4);//world 240-243, 247-252

	float w = 0.0f;
	to[0] = mvp[0] * world._14 + mvp[1] * world._24 + mvp[2] * world._34 + mvp[3];
	to[1] = mvp[4] * world._14 + mvp[5] * world._24 + mvp[6] * world._34 + mvp[7];
	//screen.z = worldToScreen.m[2][0] * point.x + worldToScreen.m[2][1] * point.y + worldToScreen.m[2][2] * point.z + worldToScreen.m[2][3];
	w = mvp[12] * world._14 + mvp[13] * world._24 + mvp[14] * world._34 + mvp[15];
	//screen.z /= w;

	if (w > 0.01f)
	{
		//aimheightxy = (Viewport.Height / 50) + (aimheight * 3);
		aimheightxy = ((float)Viewport.Height * 0.02f) + (aimheight * 3);

		float invw = 1.0f / w;
		to[0] *= invw;
		to[1] *= invw;

		float x = Viewport.Width / 2.0f;
		float y = Viewport.Height / 2.0f;

		x += 0.5f * to[0] * Viewport.Width + 0.5f;
		y -= 0.5f * to[1] * Viewport.Height - aimheightxy;
		to[0] = x + Viewport.X;
		to[1] = y + Viewport.Y;
	}
	else
	{
		to[0] = -1.0f;
		to[1] = -1.0f;
	}

	//to[0] = X
	//to[1] = Y

	//EspHPBarInfo_t pEspHPBarInfo = { static_cast<float>(to[0] + (Viewport.Width*0.5f)), static_cast<float>(to[1] + (Viewport.Height*0.5f)), iTeam };
	EspHPBarInfo_t pEspHPBarInfo = { static_cast<float>(to[0]), static_cast<float>(to[1]), iTeam };
	EspHPBarInfo.push_back(pEspHPBarInfo);

	/*
	//esp
	aimheightxy = 47 + (aimheight*3.0f);

	//Device->GetViewport(&Viewport);
	D3DXMATRIX pProjection, pView, pWorld;
	D3DXVECTOR3 vOut(0, 0, 0), vIn(0, 0, aimheightxy);

	Device->GetVertexShaderConstantF(0, pProjection, 4);
	Device->GetVertexShaderConstantF(231, pView, 4);//231
	//Device->GetVertexShaderConstantF(countnum, pWorld, 3);

	D3DXMatrixIdentity(&pWorld);

	//D3DXVECTOR3 VectorMiddle(0, 0, 0), ScreenMiddlee(Viewport.Width / 2.0f, Viewport.Height / 2.0f, 0);
	//D3DXVec3Unproject(&VectorMiddle, &ScreenMiddlee, &Viewport, &pProjection, &pView, &pWorld);

	D3DXVec3Project(&vOut, &vIn, &Viewport, &pProjection, &pView, &pWorld);

	//if (vOut.z < 1.0f)
	//{
	//float RealDistance = pProjection._44; //GetDistance(VectorMiddle.x, VectorMiddle.y, vIn.x, vIn.y);

	//AimHPBarInfo_t pAimHPBarInfo = { static_cast<float>(vOut.x + (Viewport.Width*0.5f)), static_cast<float>(vOut.y + (Viewport.Height*0.5f)), iTeam };//pSize 164 (outline)
	AimHPBarInfo_t pAimHPBarInfo = { static_cast<float>(vOut.x), static_cast<float>(vOut.y), iTeam }; //pSize 136 (glow)
	AimHPBarInfo.push_back(pAimHPBarInfo);
	*/
}

//==========================================================================================================================

struct AimTBarInfo_t
{
	float vOutX, vOutY;
	INT       iTeam;
	float CrosshairDst;
};
std::vector<AimTBarInfo_t>AimTBarInfo;

void TBarAim(LPDIRECT3DDEVICE9 Device, int iTeam)
{
	D3DXVECTOR3 from, to;
	D3DXMATRIX mvp, world;

	Device->GetVertexShaderConstantF(6, mvp, 4);//mvp

	float w = 0.0f;
	to[0] = mvp[0] * world._14 + mvp[1] * world._24 + mvp[2] * world._34 + mvp[3];
	to[1] = mvp[4] * world._14 + mvp[5] * world._24 + mvp[6] * world._34 + mvp[7];
	w = mvp[12] * world._14 + mvp[13] * world._24 + mvp[14] * world._34 + mvp[15];

	if (w > 0.01f)
	{
		//aimheightxy = (Viewport.Height / 50) + (aimheight * 5);
		aimheightxy = ((float)Viewport.Height * 0.02f) + (aimheight * 5);

		float invw = 1.0f / w;
		to[0] *= invw;
		to[1] *= invw;

		float x = Viewport.Width / 2.0f;
		float y = Viewport.Height / 2.0f;

		x += 0.5f * to[0] * Viewport.Width + 0.5f;
		y -= 0.5f * to[1] * Viewport.Height - aimheightxy;
		to[0] = x + Viewport.X;
		to[1] = y + Viewport.Y;
	}
	else
	{
		to[0] = -1.0f;
		to[1] = -1.0f;
	}

	//to[0] = X
	//to[1] = Y

	AimTBarInfo_t pAimTBarInfo = { static_cast<float>(to[0]), static_cast<float>(to[1]), iTeam };
	AimTBarInfo.push_back(pAimTBarInfo);

	/*
	float xx, yy;
	D3DXMATRIX matrix, m1;
	D3DXVECTOR4 position, input;
	Device->GetVertexShaderConstantF(6, m1, 4);//6

	D3DXMatrixTranspose(&matrix, &m1);
	//D3DXVec4Transform(&position, &input, &matrix);

	position.x = input.x * matrix._11 + input.y * matrix._21 + input.z * matrix._31 + matrix._41;
	position.y = input.x * matrix._12 + input.y * matrix._22 + input.z * matrix._32 + matrix._42;
	position.z = input.x * matrix._13 + input.y * matrix._23 + input.z * matrix._33 + matrix._43;
	position.w = input.x * matrix._14 + input.y * matrix._24 + input.z * matrix._34 + matrix._44;

	xx = ((position.x / position.w) * (Viewport.Width / 2.0f)) + Viewport.X + (Viewport.Width / 2.0f);
	yy = Viewport.Y + (Viewport.Height / 1.8f) - ((position.y / position.w) * (Viewport.Height / 1.8f));

	AimTBarInfo_t pAimTBarInfo = { static_cast<float>(xx), static_cast<float>(yy), iTeam };
	AimTBarInfo.push_back(pAimTBarInfo);
	*/
}

//==========================================================================================================================

//-----------------------------------------------------------------------------
// Name: Save()
// Desc: Saves mensa Item states for later Restoration
//-----------------------------------------------------------------------------
/*
void Save(char* szSection, char* szKey, int iValue, LPCSTR file)
{
char szValue[255];
sprintf_s(szValue, "%d", iValue);
WritempubateProfileString(szSection, szKey, szValue, file);
}
*/
//-----------------------------------------------------------------------------
// Name: Load()
// Desc: Loads mensa Item States From Previously Saved File
//-----------------------------------------------------------------------------
/*
int Load(char* szSection, char* szKey, int iDefaultValue, LPCSTR file)
{
int iResult = GetmpubateProfileInt(szSection, szKey, iDefaultValue, file);
return iResult;
}
*/

#include <string>
#include <fstream>
void SavempubCfg()
{
	ofstream fout;
	fout.open(GetmpubDirFile("mpubad3d.ini"), ios::trunc);
	fout << "Wallhack " << wallhack << endl;
	fout << "Occlusion " << occlusion << endl;
	fout << "Esp " << esp << endl;
	fout << "Aimbot " << aimbot << endl;
	fout << "Aimkey " << aimkey << endl;
	fout << "Aimsens " << aimsens << endl;
	fout << "Aimfov " << aimfov << endl;
	fout << "Aimheight " << aimheight << endl;
	fout << "AimHuman " << useaimhuman << endl;
	fout << "Autoshoot " << autoshoot << endl;
	fout << "Killsounds " << killsounds << endl;
	fout.close();
}

void LoadmpubCfg()
{
	ifstream fin;
	string Word = "";
	fin.open(GetmpubDirFile("mpubad3d.ini"), ifstream::in);
	fin >> Word >> wallhack;
	fin >> Word >> occlusion;
	fin >> Word >> esp;
	fin >> Word >> aimbot;
	fin >> Word >> aimkey;
	fin >> Word >> aimsens;
	fin >> Word >> aimfov;
	fin >> Word >> aimheight;
	fin >> Word >> useaimhuman;
	fin >> Word >> autoshoot;
	fin >> Word >> killsounds;
	fin.close();
}

//==========================================================================================================================
#define White				D3DCOLOR_ARGB(255, 255, 255, 255)
#define Yellow				D3DCOLOR_ARGB(255, 255, 255, 0)
#define TBlack				D3DCOLOR_ARGB(180, 0, 0, 0) 
#define Black				D3DCOLOR_ARGB(255, 0, 0, 0) 
#define Red					D3DCOLOR_ARGB(255, 255, 0, 0)
#define Green				D3DCOLOR_ARGB(255, 0, 255, 0)
#define DarkOutline			D3DCOLOR_ARGB(255, 37, 48, 52)

// mensa

int mensaSelect = 0;
int Current = true;

int PosX = 30;
int PosY = 27;

int Show = false; //off by default

POINT mpubPos;

#define ItemColorOn Green
#define ItemColorOff Red
#define ItemCurrent White
#define GroupColor Yellow
#define KategorieFarbe Yellow
#define ItemText White

LPD3DXFONT mpubFont; //font

int CheckTab(int x, int y, int w, int h)
{
	if (Show)
	{
		GetCursorPos(&mpubPos);
		ScreenToClient(GetForegroundWindow(), &mpubPos);
		if (mpubPos.x > x && mpubPos.x < x + w && mpubPos.y > y && mpubPos.y < y + h)
		{
			if (GetAsyncKeyState(VK_LBUTTON) & 1)
			{
				//return 1; //disabled mouse selection in menu
			}
			return 2;
		}
	}
	return 0;
}

void FillRG(LPDIRECT3DDEVICE9 pDevice, int x, int y, int w, int h, D3DCOLOR color)
{
	D3DRECT rec = { x, y, x + w, y + h };
	pDevice->Clear(1, &rec, D3DCLEAR_TARGET, color, 0, 0);
}

HRESULT DrawRectangl(LPDIRECT3DDEVICE9 Device, FLOAT x, FLOAT y, FLOAT w, FLOAT h, DWORD Color)
{
	HRESULT hRet;

	const DWORD D3D_FVF = (D3DFVF_XYZRHW | D3DFVF_DIFFUSE);

	struct Vertex
	{
		float x, y, z, ht;
		DWORD vcolor;
	}
	V[4] =
	{
		{ x, (y + h), 0.0f, 0.0f, Color },
		{ x, y, 0.0f, 0.0f, Color },
		{ (x + w), (y + h), 0.0f, 0.0f, Color },
		{ (x + w), y, 0.0f, 0.0f, Color }
	};

	hRet = D3D_OK;

	if (SUCCEEDED(hRet))
	{
		Device->SetPixelShader(0); //fix black color
		Device->SetRenderState(D3DRS_ALPHABLENDENABLE, true);
		Device->SetFVF(D3D_FVF);
		Device->SetTexture(0, NULL);
		hRet = Device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, V, sizeof(Vertex));
	}

	return hRet;
}

VOID DrawBorder(LPDIRECT3DDEVICE9 Device, INT x, INT y, INT w, INT h, INT px, DWORD BorderColor)
{
	DrawRectangl(Device, x, (y + h - px), w, px, BorderColor);
	DrawRectangl(Device, x, y, px, h, BorderColor);
	DrawRectangl(Device, x, y, w, px, BorderColor);
	DrawRectangl(Device, (x + w - px), y, px, h, BorderColor);
}

VOID DrawBoxWithBorder(LPDIRECT3DDEVICE9 Device, INT x, INT y, INT w, INT h, DWORD BoxColor, DWORD BorderColor)
{
	DrawRectangl(Device, x, y, w, h, BoxColor);
	DrawBorder(Device, x, y, w, h, 1, BorderColor);
}

VOID DrawBox(LPDIRECT3DDEVICE9 Device, INT x, INT y, INT w, INT h, DWORD BoxColor)
{
	DrawBorder(Device, x, y, w, h, 1, BoxColor);
}

void WritempubTex(int x, int y, DWORD color, char *text)
{
	RECT rect;
	SetRect(&rect, x, y, x, y);
	mpubFont->DrawText(0, text, -1, &rect, DT_NOCLIP | DT_LEFT, color);
}

void lWritempubTex(int x, int y, DWORD color, char *text)
{
	RECT rect;
	SetRect(&rect, x, y, x, y);
	mpubFont->DrawText(0, text, -1, &rect, DT_NOCLIP | DT_RIGHT, color);
}

void cWritempubTex(int x, int y, DWORD color, char *text)
{
	RECT rect;
	SetRect(&rect, x, y, x, y);
	mpubFont->DrawText(0, text, -1, &rect, DT_NOCLIP | DT_CENTER, color);
}

HRESULT DrawmpubStrin(LPD3DXFONT mpubFont, INT X, INT Y, DWORD dColor, CONST PCHAR cString, ...)
{
	HRESULT hRet;

	CHAR buf[512] = { NULL };
	va_list ArgumentList;
	va_start(ArgumentList, cString);
	_vsnprintf_s(buf, sizeof(buf), sizeof(buf) - strlen(buf), cString, ArgumentList);
	va_end(ArgumentList);

	RECT rc[2];
	SetRect(&rc[0], X, Y, X, 0);
	SetRect(&rc[1], X, Y, X + 50, 50);

	hRet = D3D_OK;

	if (SUCCEEDED(hRet))
	{
		mpubFont->DrawTextA(NULL, buf, -1, &rc[0], DT_NOCLIP, 0xFF000000);
		hRet = mpubFont->DrawTextA(NULL, buf, -1, &rc[1], DT_NOCLIP, dColor);
	}

	return hRet;
}

void Categor(LPDIRECT3DDEVICE9 pDevice, char *text)
{
	if (Show)
	{
		int Check = CheckTab(PosX+44, (PosY+51) + (Current * 15), 190, 10);
		DWORD ColorText;

		ColorText = KategorieFarbe;

		if (Check == 2)
			ColorText = ItemCurrent;

		if (mensaSelect == Current)
			ColorText = ItemCurrent;

		WritempubTex(PosX + 44, PosY+50 + (Current * 15) - 1, ColorText, text);
		lWritempubTex(PosX + 236, PosY+50 + (Current * 15) - 1, ColorText, "[-]");
		Current++;
	}
}

void AddmpubIte(LPDIRECT3DDEVICE9 pDevice, char *text, int &var, char **opt, int MaxValue)
{
	if (Show)
	{
		int Check = CheckTab(PosX+44, (PosY+51) + (Current * 15), 190, 10);
		DWORD ColorText;

		if (var)
		{
			//DrawBox(pDevice, PosX+44, PosY+51 + (Current * 15), 10, 10, Green);
			ColorText = ItemColorOn;
		}
		if (var == 0)
		{
			//DrawBox(pDevice, PosX+44, PosY+51 + (Current * 15), 10, 10, Red);
			ColorText = ItemColorOff;
		}

		if (Check == 1)
		{
			var++;
			if (var > MaxValue)
				var = 0;
		}

		if (Check == 2)
			ColorText = ItemCurrent;

		if (mensaSelect == Current)
		{
			if (GetAsyncKeyState(VK_RIGHT) & 1)
			{
				var++;
				if (var > MaxValue)
					var = 0;
			}
			else if (GetAsyncKeyState(VK_LEFT) & 1)
			{
				var--;
				if (var < 0)
					var = MaxValue;
			}
		}

		if (mensaSelect == Current)
			ColorText = ItemCurrent;


		WritempubTex(PosX + 44, PosY + 50 + (Current * 15) - 1, Black, text);
		WritempubTex(PosX + 45, PosY + 51 + (Current * 15) - 1, ColorText, text);

		lWritempubTex(PosX + 236, PosY + 50 + (Current * 15) - 1, Black, opt[var]);
		lWritempubTex(PosX + 237, PosY + 51 + (Current * 15) - 1, ColorText, opt[var]);
		Current++;
	}
}

//=====================================================================================================================

LPD3DXSPRITE mpubSprite0, mpubSprite1, mpubSprite2, mpubSprite3, mpubSprite4, mpubSprite5, mpubSprite6, mpubSprite7, mpubSprite8, mpubSprite9, mpubSprite10, mpubSprite11, mpubSprite12, mpubSprite13 = NULL; //mensa
LPD3DXSPRITE lpEsp0, lpEsp1, lpEsp2, lpEsp3, lpEsp4, lpEsp5, lpEsp6, lpEsp7, lpEsp8, lpEsp9, lpEsp10, lpEsp11, lpEsp12, lpEsp13, lpEsp14 = NULL; //esp
LPDIRECT3DTEXTURE9 mpubSpriteImage0, mpubSpriteImage1, mpubSpriteImage2, mpubSpriteImage3, mpubSpriteImage4, mpubSpriteImage5, mpubSpriteImage6, mpubSpriteImage7, mpubSpriteImage8, mpubSpriteImage9, mpubSpriteImage10, mpubSpriteImage11, mpubSpriteImage12, mpubSpriteImage13, mpubSpriteImage14 = NULL; //mensa
LPDIRECT3DTEXTURE9 lpEspImage0, lpEspImage1, lpEspImage2, lpEspImage3, lpEspImage4, lpEspImage5, lpEspImage6, lpEspImage7, lpEspImage8, lpEspImage9, lpEspImage10, lpEspImage11, lpEspImage12, lpEspImage13, lpEspImage14, lpEspImage15 = NULL; //esp
bool SpritempubCreate_d1, SpritempubCreate_d2 = false;

bool mpubaCreateSmallSprite(IDirect3DDevice9* pd3dDevice)
{
	
	HRESULT hr;

	hr = D3DXCreateTextureFromFile(pd3dDevice, GetmpubDirFile("stuff\\animations\\menu\\Menu0.png"), &mpubSpriteImage0);
	hr = D3DXCreateTextureFromFile(pd3dDevice, GetmpubDirFile("stuff\\animations\\greenwater\\Frame1.png"), &mpubSpriteImage1);
	hr = D3DXCreateTextureFromFile(pd3dDevice, GetmpubDirFile("stuff\\animations\\greenwater\\Frame2.png"), &mpubSpriteImage2);
	hr = D3DXCreateTextureFromFile(pd3dDevice, GetmpubDirFile("stuff\\animations\\greenwater\\Frame3.png"), &mpubSpriteImage3);
	hr = D3DXCreateTextureFromFile(pd3dDevice, GetmpubDirFile("stuff\\animations\\greenwater\\Frame4.png"), &mpubSpriteImage4);
	hr = D3DXCreateTextureFromFile(pd3dDevice, GetmpubDirFile("stuff\\animations\\greenwater\\Frame5.png"), &mpubSpriteImage5);
	hr = D3DXCreateTextureFromFile(pd3dDevice, GetmpubDirFile("stuff\\animations\\greenwater\\Frame6.png"), &mpubSpriteImage6);
	hr = D3DXCreateTextureFromFile(pd3dDevice, GetmpubDirFile("stuff\\animations\\greenwater\\Frame7.png"), &mpubSpriteImage7);
	hr = D3DXCreateTextureFromFile(pd3dDevice, GetmpubDirFile("stuff\\animations\\greenwater\\Frame8.png"), &mpubSpriteImage8);
	hr = D3DXCreateTextureFromFile(pd3dDevice, GetmpubDirFile("stuff\\animations\\greenwater\\Frame9.png"), &mpubSpriteImage9);
	hr = D3DXCreateTextureFromFile(pd3dDevice, GetmpubDirFile("stuff\\animations\\greenwater\\Frame10.png"), &mpubSpriteImage10);
	hr = D3DXCreateTextureFromFile(pd3dDevice, GetmpubDirFile("stuff\\animations\\greenwater\\Frame11.png"), &mpubSpriteImage11);
	hr = D3DXCreateTextureFromFile(pd3dDevice, GetmpubDirFile("stuff\\animations\\greenwater\\Frame12.png"), &mpubSpriteImage12);
	hr = D3DXCreateTextureFromFile(pd3dDevice, GetmpubDirFile("stuff\\animations\\greenwater\\Frame13.png"), &mpubSpriteImage13);

	if (FAILED(hr))
	{
		//Log("D3DXCreateTextureFromFile failed");
		//bSpriteCreated1 = detected?
		SpritempubCreate_d1 = false;
		return false;
	}

	hr = D3DXCreateSprite(pd3dDevice, &mpubSprite0);
	hr = D3DXCreateSprite(pd3dDevice, &mpubSprite1);
	hr = D3DXCreateSprite(pd3dDevice, &mpubSprite2);
	hr = D3DXCreateSprite(pd3dDevice, &mpubSprite3);
	hr = D3DXCreateSprite(pd3dDevice, &mpubSprite4);
	hr = D3DXCreateSprite(pd3dDevice, &mpubSprite5);
	hr = D3DXCreateSprite(pd3dDevice, &mpubSprite6);
	hr = D3DXCreateSprite(pd3dDevice, &mpubSprite7);
	hr = D3DXCreateSprite(pd3dDevice, &mpubSprite8);
	hr = D3DXCreateSprite(pd3dDevice, &mpubSprite9);
	hr = D3DXCreateSprite(pd3dDevice, &mpubSprite10);
	hr = D3DXCreateSprite(pd3dDevice, &mpubSprite11);
	hr = D3DXCreateSprite(pd3dDevice, &mpubSprite12);
	hr = D3DXCreateSprite(pd3dDevice, &mpubSprite13);

	if (FAILED(hr))
	{
		//Log("D3DXCreateSprite1 failed");
		SpritempubCreate_d1 = false;
		return false;
	}

	SpritempubCreate_d1 = true;
	
	return true;
}

bool mpubaCreateSmallSprite2(IDirect3DDevice9* pd3dDevice)
{
	
	HRESULT hr;

	hr = D3DXCreateTextureFromFile(pd3dDevice, GetmpubDirFile("stuff\\animations\\blackbluecircle\\Frame0.png"), &lpEspImage0);
	hr = D3DXCreateTextureFromFile(pd3dDevice, GetmpubDirFile("stuff\\animations\\blackbluecircle\\Frame1.png"), &lpEspImage1);
	hr = D3DXCreateTextureFromFile(pd3dDevice, GetmpubDirFile("stuff\\animations\\blackbluecircle\\Frame2.png"), &lpEspImage2);
	hr = D3DXCreateTextureFromFile(pd3dDevice, GetmpubDirFile("stuff\\animations\\blackbluecircle\\Frame3.png"), &lpEspImage3);
	hr = D3DXCreateTextureFromFile(pd3dDevice, GetmpubDirFile("stuff\\animations\\blackbluecircle\\Frame4.png"), &lpEspImage4);
	hr = D3DXCreateTextureFromFile(pd3dDevice, GetmpubDirFile("stuff\\animations\\blackbluecircle\\Frame5.png"), &lpEspImage5);
	hr = D3DXCreateTextureFromFile(pd3dDevice, GetmpubDirFile("stuff\\animations\\blackbluecircle\\Frame6.png"), &lpEspImage6);
	hr = D3DXCreateTextureFromFile(pd3dDevice, GetmpubDirFile("stuff\\animations\\blackbluecircle\\Frame7.png"), &lpEspImage7);
	hr = D3DXCreateTextureFromFile(pd3dDevice, GetmpubDirFile("stuff\\animations\\blackbluecircle\\Frame8.png"), &lpEspImage8);
	hr = D3DXCreateTextureFromFile(pd3dDevice, GetmpubDirFile("stuff\\animations\\blackbluecircle\\Frame9.png"), &lpEspImage9);
	hr = D3DXCreateTextureFromFile(pd3dDevice, GetmpubDirFile("stuff\\animations\\blackbluecircle\\Frame10.png"), &lpEspImage10);
	hr = D3DXCreateTextureFromFile(pd3dDevice, GetmpubDirFile("stuff\\animations\\blackbluecircle\\Frame11.png"), &lpEspImage11);
	hr = D3DXCreateTextureFromFile(pd3dDevice, GetmpubDirFile("stuff\\animations\\blackbluecircle\\Frame12.png"), &lpEspImage12);
	hr = D3DXCreateTextureFromFile(pd3dDevice, GetmpubDirFile("stuff\\animations\\blackbluecircle\\Frame13.png"), &lpEspImage13);
	hr = D3DXCreateTextureFromFile(pd3dDevice, GetmpubDirFile("stuff\\animations\\blackbluecircle\\Frame14.png"), &lpEspImage14);
	
	if (FAILED(hr))
	{
		//Log("D3DXCreateTextureFromFile failed");
		//bSpriteCreated2 = detected?
		SpritempubCreate_d2 = false;
		return false;
	}

	hr = D3DXCreateSprite(pd3dDevice, &lpEsp0);
	hr = D3DXCreateSprite(pd3dDevice, &lpEsp1);
	hr = D3DXCreateSprite(pd3dDevice, &lpEsp2);
	hr = D3DXCreateSprite(pd3dDevice, &lpEsp3);
	hr = D3DXCreateSprite(pd3dDevice, &lpEsp4);
	hr = D3DXCreateSprite(pd3dDevice, &lpEsp5);
	hr = D3DXCreateSprite(pd3dDevice, &lpEsp6);
	hr = D3DXCreateSprite(pd3dDevice, &lpEsp7);
	hr = D3DXCreateSprite(pd3dDevice, &lpEsp8);
	hr = D3DXCreateSprite(pd3dDevice, &lpEsp9);
	hr = D3DXCreateSprite(pd3dDevice, &lpEsp10);
	hr = D3DXCreateSprite(pd3dDevice, &lpEsp11);
	hr = D3DXCreateSprite(pd3dDevice, &lpEsp12);
	hr = D3DXCreateSprite(pd3dDevice, &lpEsp13);
	hr = D3DXCreateSprite(pd3dDevice, &lpEsp14);

	if (FAILED(hr))
	{
		//Log("D3DXCreateSprite2 failed");
		SpritempubCreate_d2 = false;
		return false;
	}

	SpritempubCreate_d2 = true;
	
	return true;
}

// COM utils
template<class COMObject>
void SafeRelease(COMObject*& pRes)
{
	IUnknown *unknown = pRes;
	if (unknown)
	{
		unknown->Release();
	}
	pRes = NULL;
}

// This will get called before Device::Clear(). If the device has been reset
// then all the work surfaces will be created again.
void PrempubClear(IDirect3DDevice9* ddevice)
{
	if (!SpritempubCreate_d1)
		mpubaCreateSmallSprite(ddevice);

	if (!SpritempubCreate_d2)
		mpubaCreateSmallSprite2(ddevice);
}

// Delete work surfaces when device gets reset
void DeleteSurfaces()
{
	if (mpubSprite0 != NULL)
	{
		//Log("SafeRelease(mpubSprite)");
		SafeRelease(mpubSprite0);
		SafeRelease(mpubSprite1);
		SafeRelease(mpubSprite2);
		SafeRelease(mpubSprite3);
		SafeRelease(mpubSprite4);
		SafeRelease(mpubSprite5);
		SafeRelease(mpubSprite6);
		SafeRelease(mpubSprite7);
		SafeRelease(mpubSprite8);
		SafeRelease(mpubSprite9);
		SafeRelease(mpubSprite10);
		SafeRelease(mpubSprite11);
		SafeRelease(mpubSprite12);
		SafeRelease(mpubSprite13);
	}

	if (lpEsp0 != NULL)
	{
		//Log("SafeRelease(lpEsp)");
		SafeRelease(lpEsp0);
		SafeRelease(lpEsp1);
		SafeRelease(lpEsp2);
		SafeRelease(lpEsp3);
		SafeRelease(lpEsp4);
		SafeRelease(lpEsp5);
		SafeRelease(lpEsp6);
		SafeRelease(lpEsp7);
		SafeRelease(lpEsp8);
		SafeRelease(lpEsp9);
		SafeRelease(lpEsp10);
		SafeRelease(lpEsp11);
		SafeRelease(lpEsp12);
		SafeRelease(lpEsp13);
		SafeRelease(lpEsp14);
	}

	SpritempubCreate_d1 = false;
	SpritempubCreate_d2 = false;
}

// This gets called right before the frame is presented on-screen - Device::Present().
// First, create the display text, FPS and info message, on-screen. Then then call
// CopySurfaceToTextureBuffer() to downsample the image and copy to shared memory
void PrempubPresent(IDirect3DDevice9* Device, int cx, int cy)
{
	int textOffsetLeft;

	//draw sprite
	if (SpritempubCreate_d1)
	{
		if (mpubSprite0 != NULL)
		{
			D3DXVECTOR3 position;
			position.x = (float)cx;
			position.y = (float)cy;
			position.z = 0.0f;

			textOffsetLeft = (int)position.x; //for later to offset text from image

			//mpubSprite1->Begin(D3DXSPRITE_ALPHABLEND);
			//mpubSprite1->Draw(mpubSpriteImage1, NULL, NULL, &position, 0xFFFFFFFF);
			//mpubSprite1->End();

			//position.x = 20.0f; //79.0f;
			//position.y = 20.0f;
			//position.z = 0.0f;

			//mensa background scale
			D3DXMATRIX scaleMatrix0;
			D3DXMATRIX transMatrix0;
			D3DXMatrixScaling(&scaleMatrix0, 1.0f, 0.6f, 1.0f);
			D3DXMatrixTranslation(&transMatrix0, position.x, position.y, position.z);
			D3DXMatrixMultiply(&transMatrix0, &scaleMatrix0, &transMatrix0);

			//draw mensa background pic
			mpubSprite0->SetTransform(&transMatrix0);
			mpubSprite0->Begin(D3DXSPRITE_ALPHABLEND);
			mpubSprite0->Draw(mpubSpriteImage0, NULL, NULL, &position, 0xFFFFFFFF);
			mpubSprite0->End();

			//anim timer
			dwTime0 = GetTickCount() / 60;//50 speed

			//setting the starttime in ms
			if (dwTime0 - dwStartTime0 > 12)
				dwStartTime0 = GetTickCount() / 60;

			position.x = 49.0f; //76.0f;
			position.y = 119.0f;
			position.z = 0.0f;

			//animation scale
			D3DXMATRIX scaleMatrix;
			D3DXMATRIX transMatrix;
			D3DXMatrixScaling(&scaleMatrix, 1.51f, 0.755f, 1.0f);
			//D3DXMatrixTranslation(&transMatrix, ImagePos0.x, ImagePos0.y, ImagePos0.z);
			D3DXMatrixTranslation(&transMatrix, 0.0f, 0.0f, 0.0f);
			D3DXMatrixMultiply(&transMatrix, &scaleMatrix, &transMatrix);

			//draw animation foreground pics
			if (dwTime0 - dwStartTime0 == 0)
			{
				mpubSprite1->SetTransform(&transMatrix);
				mpubSprite1->Begin(D3DXSPRITE_ALPHABLEND);
				mpubSprite1->Draw(mpubSpriteImage1, NULL, NULL, &position, 0xFFFFFFFF);
				mpubSprite1->End();
			}

			if (dwTime0 - dwStartTime0 == 1)
			{
				mpubSprite2->SetTransform(&transMatrix);
				mpubSprite2->Begin(D3DXSPRITE_ALPHABLEND);
				mpubSprite2->Draw(mpubSpriteImage2, NULL, NULL, &position, 0xFFFFFFFF);
				mpubSprite2->End();
			}

			if (dwTime0 - dwStartTime0 == 2)
			{
				mpubSprite3->SetTransform(&transMatrix);
				mpubSprite3->Begin(D3DXSPRITE_ALPHABLEND);
				mpubSprite3->Draw(mpubSpriteImage3, NULL, NULL, &position, 0xFFFFFFFF);
				mpubSprite3->End();
			}

			if (dwTime0 - dwStartTime0 == 3)
			{
				mpubSprite4->SetTransform(&transMatrix);
				mpubSprite4->Begin(D3DXSPRITE_ALPHABLEND);
				mpubSprite4->Draw(mpubSpriteImage4, NULL, NULL, &position, 0xFFFFFFFF);
				mpubSprite4->End();
			}

			if (dwTime0 - dwStartTime0 == 4)
			{
				mpubSprite5->SetTransform(&transMatrix);
				mpubSprite5->Begin(D3DXSPRITE_ALPHABLEND);
				mpubSprite5->Draw(mpubSpriteImage5, NULL, NULL, &position, 0xFFFFFFFF);
				mpubSprite5->End();
			}

			if (dwTime0 - dwStartTime0 == 5)
			{
				mpubSprite6->SetTransform(&transMatrix);
				mpubSprite6->Begin(D3DXSPRITE_ALPHABLEND);
				mpubSprite6->Draw(mpubSpriteImage6, NULL, NULL, &position, 0xFFFFFFFF);
				mpubSprite6->End();
			}

			if (dwTime0 - dwStartTime0 == 6)
			{
				mpubSprite7->SetTransform(&transMatrix);
				mpubSprite7->Begin(D3DXSPRITE_ALPHABLEND);
				mpubSprite7->Draw(mpubSpriteImage7, NULL, NULL, &position, 0xFFFFFFFF);
				mpubSprite7->End();
			}

			if (dwTime0 - dwStartTime0 == 7)
			{
				mpubSprite8->SetTransform(&transMatrix);
				mpubSprite8->Begin(D3DXSPRITE_ALPHABLEND);
				mpubSprite8->Draw(mpubSpriteImage8, NULL, NULL, &position, 0xFFFFFFFF);
				mpubSprite8->End();
			}

			if (dwTime0 - dwStartTime0 == 8)
			{
				mpubSprite9->SetTransform(&transMatrix);
				mpubSprite9->Begin(D3DXSPRITE_ALPHABLEND);
				mpubSprite9->Draw(mpubSpriteImage9, NULL, NULL, &position, 0xFFFFFFFF);
				mpubSprite9->End();
			}

			if (dwTime0 - dwStartTime0 == 9)
			{
				mpubSprite10->SetTransform(&transMatrix);
				mpubSprite10->Begin(D3DXSPRITE_ALPHABLEND);
				mpubSprite10->Draw(mpubSpriteImage10, NULL, NULL, &position, 0xFFFFFFFF);
				mpubSprite10->End();
			}

			if (dwTime0 - dwStartTime0 == 10)
			{
				mpubSprite11->SetTransform(&transMatrix);
				mpubSprite11->Begin(D3DXSPRITE_ALPHABLEND);
				mpubSprite11->Draw(mpubSpriteImage11, NULL, NULL, &position, 0xFFFFFFFF);
				mpubSprite11->End();
			}

			if (dwTime0 - dwStartTime0 == 11)
			{
				mpubSprite12->SetTransform(&transMatrix);
				mpubSprite12->Begin(D3DXSPRITE_ALPHABLEND);
				mpubSprite12->Draw(mpubSpriteImage12, NULL, NULL, &position, 0xFFFFFFFF);
				mpubSprite12->End();
			}

			if (dwTime0 - dwStartTime0 == 12)
			{
				mpubSprite13->SetTransform(&transMatrix);
				mpubSprite13->Begin(D3DXSPRITE_ALPHABLEND);
				mpubSprite13->Draw(mpubSpriteImage13, NULL, NULL, &position, 0xFFFFFFFF);
				mpubSprite13->End();
			}
		}
	}

	// draw text
}

void PrempubPresent2(IDirect3DDevice9* Device, int cx, int cy)
{
	int textOffsetLeft;

	//draw sprite
	if (SpritempubCreate_d2)
	{
		if (lpEsp0 != NULL)
		{
			D3DXVECTOR3 position;
			position.x = (float)cx;
			position.y = (float)cy;
			position.z = 0.0f;

			textOffsetLeft = (int)position.x; //for later to offset text from image

			//draw esp background pic
			//lpEsp0->SetTransform(&transMatrix0);
			lpEsp0->Begin(D3DXSPRITE_ALPHABLEND);
			lpEsp0->Draw(lpEspImage0, NULL, NULL, &position, 0xFFFFFFFF);
			lpEsp0->End();


			//anim timer
			dwTime1 = GetTickCount() / 60;//50 speed

			//setting the starttime in ms
			if (dwTime1 - dwStartTime1 > 13)
				dwStartTime1 = GetTickCount() / 60;

		
			//draw animation foreground pics
			if (dwTime1 - dwStartTime1 == 0)
			{
				//lpEsp1->SetTransform(&transMatrix);
				lpEsp1->Begin(D3DXSPRITE_ALPHABLEND);
				lpEsp1->Draw(lpEspImage1, NULL, NULL, &position, 0xFFFFFFFF);
				lpEsp1->End();
			}

			if (dwTime1 - dwStartTime1 == 1)
			{
				//lpEsp2->SetTransform(&transMatrix);
				lpEsp2->Begin(D3DXSPRITE_ALPHABLEND);
				lpEsp2->Draw(lpEspImage2, NULL, NULL, &position, 0xFFFFFFFF);
				lpEsp2->End();
			}

			if (dwTime1 - dwStartTime1 == 2)
			{
				//lpEsp3->SetTransform(&transMatrix);
				lpEsp3->Begin(D3DXSPRITE_ALPHABLEND);
				lpEsp3->Draw(lpEspImage3, NULL, NULL, &position, 0xFFFFFFFF);
				lpEsp3->End();
			}

			if (dwTime1 - dwStartTime1 == 3)
			{
				//lpEsp4->SetTransform(&transMatrix);
				lpEsp4->Begin(D3DXSPRITE_ALPHABLEND);
				lpEsp4->Draw(lpEspImage4, NULL, NULL, &position, 0xFFFFFFFF);
				lpEsp4->End();
			}

			if (dwTime1 - dwStartTime1 == 4)
			{
				//lpEsp5->SetTransform(&transMatrix);
				lpEsp5->Begin(D3DXSPRITE_ALPHABLEND);
				lpEsp5->Draw(lpEspImage5, NULL, NULL, &position, 0xFFFFFFFF);
				lpEsp5->End();
			}

			if (dwTime1 - dwStartTime1 == 5)
			{
				//lpEsp6->SetTransform(&transMatrix);
				lpEsp6->Begin(D3DXSPRITE_ALPHABLEND);
				lpEsp6->Draw(lpEspImage6, NULL, NULL, &position, 0xFFFFFFFF);
				lpEsp6->End();
			}

			if (dwTime1 - dwStartTime1 == 6)
			{
				//lpEsp7->SetTransform(&transMatrix);
				lpEsp7->Begin(D3DXSPRITE_ALPHABLEND);
				lpEsp7->Draw(lpEspImage7, NULL, NULL, &position, 0xFFFFFFFF);
				lpEsp7->End();
			}

			if (dwTime1 - dwStartTime1 == 7)
			{
				//lpEsp8->SetTransform(&transMatrix);
				lpEsp8->Begin(D3DXSPRITE_ALPHABLEND);
				lpEsp8->Draw(lpEspImage8, NULL, NULL, &position, 0xFFFFFFFF);
				lpEsp8->End();
			}

			if (dwTime1 - dwStartTime1 == 8)
			{
				//lpEsp9->SetTransform(&transMatrix);
				lpEsp9->Begin(D3DXSPRITE_ALPHABLEND);
				lpEsp9->Draw(lpEspImage9, NULL, NULL, &position, 0xFFFFFFFF);
				lpEsp9->End();
			}

			if (dwTime1 - dwStartTime1 == 9)
			{
				//lpEsp10->SetTransform(&transMatrix);
				lpEsp10->Begin(D3DXSPRITE_ALPHABLEND);
				lpEsp10->Draw(lpEspImage10, NULL, NULL, &position, 0xFFFFFFFF);
				lpEsp10->End();
			}

			if (dwTime1 - dwStartTime1 == 10)
			{
				//lpEsp11->SetTransform(&transMatrix);
				lpEsp11->Begin(D3DXSPRITE_ALPHABLEND);
				lpEsp11->Draw(lpEspImage11, NULL, NULL, &position, 0xFFFFFFFF);
				lpEsp11->End();
			}

			if (dwTime1 - dwStartTime1 == 11)
			{
				//lpEsp12->SetTransform(&transMatrix);
				lpEsp12->Begin(D3DXSPRITE_ALPHABLEND);
				lpEsp12->Draw(lpEspImage12, NULL, NULL, &position, 0xFFFFFFFF);
				lpEsp12->End();
			}

			if (dwTime1 - dwStartTime1 == 12)
			{
				//lpEsp13->SetTransform(&transMatrix);
				lpEsp13->Begin(D3DXSPRITE_ALPHABLEND);
				lpEsp13->Draw(lpEspImage13, NULL, NULL, &position, 0xFFFFFFFF);
				lpEsp13->End();
			}

			if (dwTime1 - dwStartTime1 == 13)
			{
				//lpEsp14->SetTransform(&transMatrix);
				lpEsp14->Begin(D3DXSPRITE_ALPHABLEND);
				lpEsp14->Draw(lpEspImage14, NULL, NULL, &position, 0xFFFFFFFF);
				lpEsp14->End();
			}
		}
	}

	// draw text
}

//==========================================================================================================================

// menu part
char *opt_OnOff[] = { "[OFF]", "[On]" };
char *opt_OnEsp[] = { "[OFF]", "[Pic]", "[Text]" };
char *opt_WhChams[] = { "[OFF]", "[On]", "[On + Glow]", "[On + Chams]" };
char *opt_Teams[] = { "[OFF]", "[Mode 1]", "[Mode 2]", "[Compatibility]" };
char *opt_Keys[] = { "[OFF]", "[Shift]", "[RMouse]", "[LMouse]", "[Ctrl]", "[Alt]", "[Space]", "[X]", "[C]" };
char *opt_Sensitivity[] = { "[1]", "[2]", "[3]", "[4]", "[5]", "[6]", "[7]", "[8]", "[9]", "[10]", "[11]", "[12]", "[13]", "[14]", "[15]", "[16]", "[17]", "[18]", "[19]", "[20]" };
char *opt_Aimheight[] = { "[0]", "[1]", "[2]", "[3]", "[4]", "[5]", "[6]", "[7]", "[8]", "[9]", "[10]" };
char *opt_Aimfov[] = { "[0]", "[10%]", "[20%]", "[30%]", "[40%]", "[50%]", "[60%]", "[70%]", "[80%]", "[90%]" };
char *opt_Aimhuman[] = { "[Off]", "[On sens 1]", "[On sens 2]", "[On sens 3]", "[On sens 4]", "[On sens 5]", "[On sens 6]", "[On sens 7]", "[On sens 8]", "[On sens 9]", "[On sens 10]" };
char *opt_autoshoot[] = { "[OFF]", "[OnKeyDown]" };

void DrawmpubMenu(LPDIRECT3DDEVICE9 pDevice)
{
	if (GetAsyncKeyState(VK_INSERT) & 1)
	{
		Show = !Show;

		//save settings
		SavempubCfg();

		//Save("wallhack", "wallhack", wallhack, GetmpubDirFile("mpubaconfig.ini"));

		PlaySoundA(GetmpubDirFile("stuff\\sounds\\menu.wav"), 0, SND_FILENAME | SND_ASYNC | SND_NOSTOP | SND_NODEFAULT);
	}

	if (Show && mpubFont)
	{
		if (GetAsyncKeyState(VK_UP) & 1)
			mensaSelect--;

		if (GetAsyncKeyState(VK_DOWN) & 1)
			mensaSelect++;

		//draw background
		FillRG(pDevice, 71, 64, 200, 201, TBlack);
		DrawBox(pDevice, 71, 63, 200, 1, DarkOutline);
		DrawBox(pDevice, 71, 86, 200, Current * 15, DarkOutline);
		//draw mensa pic
		PrempubPresent(pDevice, 20, 20);
		//cWritempubTex(172, 71, White, "Paladins D3D");

		Current = 1;
		//Categor(pDevice, " [D3D]");
		AddmpubIte(pDevice, " Wallhack", wallhack, opt_WhChams, 3);
		AddmpubIte(pDevice, " Occlusion", occlusion, opt_OnOff, 1);
		AddmpubIte(pDevice, " Esp", esp, opt_OnEsp, 2);
		AddmpubIte(pDevice, " Aimbot", aimbot, opt_Teams, 3);
		AddmpubIte(pDevice, " Aimkey", aimkey, opt_Keys, 8);
		AddmpubIte(pDevice, " Aimsensitivity", aimsens, opt_Sensitivity, 19);
		AddmpubIte(pDevice, " Aimfov", aimfov, opt_Aimfov, 9);
		AddmpubIte(pDevice, " Aimheight", aimheight, opt_Aimheight, 10);
		AddmpubIte(pDevice, " AimHuman", useaimhuman, opt_Aimhuman, 10);
		AddmpubIte(pDevice, " Autoshoot", autoshoot, opt_autoshoot, 1);
		AddmpubIte(pDevice, " Killsounds", killsounds, opt_OnOff, 1);

		if (mensaSelect >= Current)
			mensaSelect = 1;

		if (mensaSelect < 1)
			mensaSelect = 11;//Current;
	}
}

//=====================================================================================================================

