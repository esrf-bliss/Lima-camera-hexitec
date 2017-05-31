// aS_lib.h

#define AS_LIB_API __declspec(dllexport)

#include "stdafx.h"

namespace aS {

	#ifdef UNICODE
	#define AS_CONST_STR L
	#else
	#define AS_CONST_STR
	#endif // !UNICODE
	
	typedef unsigned char					u8,				*p_u8;
	typedef char							i8,				*p_i8;
	typedef unsigned short					u16,			*p_u16;
	typedef short							i16,			*p_i16;
	typedef unsigned int					u32,			*p_u32;
	typedef int								i32,			*p_i32;
	typedef unsigned __int64				u64,			*p_u64;
	typedef __int64							i64,			*p_i64;
	typedef float							sgl,			*p_sgl;
	typedef double							dbl,			*p_dbl;
	typedef char							char8,			*p_char8;
	typedef wchar_t							char16,			*p_char16;
	typedef TCHAR							charSys,		*p_charSys;
	typedef LPSTR							str8,			*p_str8;
	typedef LPWSTR							str16,			*p_str16;
	typedef LPTSTR							strSys,			*p_strSys;
	typedef std::string						string8,		*p_string8;
	typedef std::wstring					string16,		*p_string16;
	typedef void*							p_image;							
	
	#ifdef UNICODE
	typedef string16						stringSys,		*p_stringSys;
	#else
	typedef string8							stringSys,		*p_stringSys;
	#endif // !UNICODE								
	
	typedef std::vector<u8>					u8_vec,			*p_u8_vec;
	typedef std::vector<i8>					i8_vec,			*p_i8_vec;
	typedef std::vector<u16>				u16_vec,		*p_u16_vec;
	typedef std::vector<i16>				i16_vec,		*p_i16_vec;
	typedef std::vector<u32>				u32_vec,		*p_u32_vec;
	typedef std::vector<i32>				i32_vec,		*p_i32_vec;
	typedef std::vector<u64>				u64_vec,		*p_u64_vec;
	typedef std::vector<i64>				i64_vec,		*p_i64_vec;
	typedef std::vector<sgl>				sgl_vec,		*p_sgl_vec;
	typedef std::vector<dbl>				dbl_vec,		*p_dbl_vec;
	typedef std::vector<char8>				char8_vec,		*p_char8_vec;
	typedef std::vector<char16>				char16_vec,		*p_char16_vec;
	typedef std::vector<charSys>			charSys_vec,	*p_charSys_vec;
	typedef std::vector<str8>				str8_vec,		*p_str8_vec;
	typedef std::vector<str16>				str16_vec,		*p_str16_vec;
	typedef std::vector<strSys>				strSys_vec,		*p_strSys_vec;
	typedef std::vector<string8>			string8_vec,	*p_string8_vec;
	typedef std::vector<string16>			string16_vec,	*p_string16_vec;
	typedef std::vector<u8_vec>				fb_vec,			*p_fb_vec;
	typedef std::vector<i8_vec>				fg_funcSel,		*p_fg_funcSel;
	typedef std::vector<p_image>			p_image_vec;

	#ifdef UNICODE
	typedef string16_vec					stringSys_vec,	*p_stringSys_vec;
	#else
	typedef string8_vec						stringSys_vec,	*p_stringSys_vec;
	#endif // !UNICODE

	typedef HANDLE							hdl,			*p_hdl;
	typedef std::vector<HANDLE>				hdl_vec,		*p_hdl_vec;

	#define AS_MESSAGE_DLL			AS_CONST_STR"aS_messages.dll"

	#define AS_FPGA_FW_CHECK_CUSTOMER_REG (u8)0x80
	#define AS_FPGA_FW_CHECK_PROJECT_REG (u8)0x81
	#define AS_FPGA_FW_CHECK_VERSION_REG (u8)0x82

	#define AS_IM_TIMEOUT 5000

// Instance Manager

	typedef struct IM_HANDLE_STRUCT{
	void			*Handle;
	hdl				Status;
	} IM_HANDLE_STRUCT, *PIM_HANDLE_STRUCT, **HIM_HANDLE_STRUCT;

	class InstanceManager
	{
	private:
		u32								TimeOut;
		std::vector<IM_HANDLE_STRUCT>	HandleVector;
		void							*VectorMutex;

	public:
		InstanceManager();
		InstanceManager(u32 TimeOut);
		~InstanceManager();

		i32 AddInstance(void *InstanceHandle);
		i32 SetInstanceBusy(void *InstanceHandle);
		i32 SetInstanceReady(void *InstanceHandle);
		i32 RemInstance(void *InstanceHandle);

	private:
		i32 FindInstance(void *InstanceHandle, u32 *Position);
	};

	typedef enum Control : u8 {
		AS_CONTROL_DISABLED			= 0,
		AS_CONTROL_ENABLED			= 1
		} Control;

	#ifndef _M_X64
	#pragma pack(push)
	#pragma pack(1)
	#endif
	
	typedef struct FpgaRegister {
		u8			address;
		u8			value;
		} FpgaRegister, *FpgaRegisterPtr, **FpgaRegisterHdl;

	#ifndef _M_X64
	#pragma pack(pop)
	#endif

	typedef std::vector<FpgaRegister>				FpgaRegister_vec,		*p_FpgaRegister_vec;
}