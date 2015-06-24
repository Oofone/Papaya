#pragma once

#define Kilobytes(Value) ((Value)*1024LL)
#define Megabytes(Value) (Kilobytes(Value)*1024LL)
#define Gigabytes(Value) (Megabytes(Value)*1024LL)
#define Terabytes(Value) (Gigabytes(Value)*1024LL)

#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))

enum PapayaInterfaceTexture_
{
	PapayaInterfaceTexture_Font,
	PapayaInterfaceTexture_TitleBarButtons,
	PapayaInterfaceTexture_TitleBarIcon,
	PapayaInterfaceTexture_InterfaceIcons,
	PapayaInterfaceTexture_COUNT
};

enum PapayaInterfaceColor_
{
	PapayaInterfaceColor_Clear,
	PapayaInterfaceColor_Transparent,
	PapayaInterfaceColor_ButtonHover,
	PapayaInterfaceColor_ButtonActive,
	PapayaInterfaceColor_COUNT
};

struct PapayaDocument
{
	uint8* Texture;
	int32 Width, Height;
	int32 ComponentsPerPixel;
	uint32 TextureID;
	ImVec2 CanvasPosition;
	float CanvasZoom;
};

struct PapayaShader
{
	int32 Handle;
	int32 Texture, ProjectionMatrix, Position, UV, Color;
};

struct PapayaGraphicsBuffers
{
	size_t VboSize;
	uint32 VboHandle, VaoHandle;
};

struct PapayaWindow
{
	uint32 Width, Height;
	float MaximizeOffset; // Used to cancel Windows' weirdness when windows are maximized. Refer: http://stackoverflow.com/questions/14667558/why-is-a-maximized-delphi-form-8-pixels-wider-and-higher-than-the-getsystemmetri
};

struct PapayaMemory
{
	uint32 InterfaceTextureIDs[PapayaInterfaceTexture_COUNT];
	ImVec4 InterfaceColors[PapayaInterfaceColor_COUNT];
	uint32 CurrentColor;
	PapayaWindow Window;
	PapayaGraphicsBuffers GraphicsBuffers;
	PapayaShader DefaultShader;
	PapayaDocument* Documents; // TODO: Use an array or vector instead of bare pointer?
};

struct PapayaDebugMemory
{
	int64 Time;
	int64 TicksPerSecond;
};