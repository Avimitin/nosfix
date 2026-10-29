__int64 __fastcall sub_180366AC0(__int64 a1)
{
  unsigned int v2; // ebx
  char v3; // si
  int v4; // eax
  char v5; // cl

  v2 = 7; /*0x180366ad2*/
  v3 = sub_180366310(a1); /*0x180366adc*/
  if ( *(_BYTE *)(sub_180311A00() + 37) == 0 ) /*0x180366ae8*/
    goto LABEL_9; /*0x180366ae8*/
  if ( (unsigned __int8)ac_io_is_active_device(a1: 26, a2: 1) != 0 /*0x180366b02*/
    && (unsigned __int8)ac_io_panb_update_control_status_buffer() != 0 )
  {
    ac_io_panb_get_control_status_buffer(a1: a1 + 2224); /*0x180366b0b*/
    v4 = 0; /*0x180366b11*/
  }
  else
  {
    v4 = dword_1813EE5C0 + 1; /*0x180366b1b*/
  }
  dword_1813EE5C0 = v4; /*0x180366b1d*/
  if ( v4 <= 600 ) /*0x180366b28*/
  {
    ac_io_update(a1: 1); /*0x180366b3a*/
LABEL_9:
    v5 = 1; /*0x180366b40*/
    goto LABEL_10; /*0x180366b40*/
  }
  *(_DWORD *)(a1 + 4) = 5; /*0x180366b2a*/
  v5 = 0; /*0x180366b31*/
LABEL_10:
  if ( v3 != 0 && v5 != 0 ) /*0x180366b4e*/
    return 6; /*0x180366b4e*/
  return v2; /*0x180366b51*/
}
