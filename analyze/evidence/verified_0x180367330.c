void __fastcall sub_180367330(void **a1)
{
  void **v1; // rdi
  int v2; // ebx
  _QWORD *v3; // rax
  __int64 v4; // rax
  __int64 v6; // rsi
  __int64 v7; // rcx
  int v8; // [rsp+28h] [rbp-50h] BYREF
  _BYTE v9[8]; // [rsp+2Ch] [rbp-4Ch] BYREF
  _BYTE v10[24]; // [rsp+48h] [rbp-30h] BYREF

  v1 = a1; /*0x18036734c*/
  if ( byte_1812CF8FA == 0 && *((_BYTE *)a1 + 8) != 0 && *((_BYTE *)a1 + 20) != 0 ) /*0x180367363*/
  {
    v2 = 0; /*0x180367371*/
    if ( qword_1812D1E70 == 0 ) /*0x18036737a*/
    {
      v3 = operator new(Size: 0x40u); /*0x18036737f*/
      a1 = &CDeviceDriver::`vftable'; /*0x180367384*/
      qword_1812D1E70 = (__int64)v3; /*0x180367390*/
      *v3 = &CDeviceDriver::`vftable'; /*0x180367397*/
      v3[1] = 0; /*0x18036739a*/
      v3[2] = 0; /*0x18036739e*/
      v3[3] = 0; /*0x1803673a2*/
      v3[4] = 0; /*0x1803673a6*/
      v3[5] = 0; /*0x1803673aa*/
      v3[6] = 0; /*0x1803673ae*/
      *((_DWORD *)v3 + 14) = 0; /*0x1803673b2*/
      *((_WORD *)v3 + 30) = 0; /*0x1803673b5*/
    }
    v4 = sub_180311A00(a1); /*0x1803673b9*/
    if ( *(_BYTE *)(v4 + 36) != 0 && *(_BYTE *)(v4 + 37) != 0 ) /*0x1803673c6*/
    {
      v6 = sub_180369760(); /*0x1803673e3*/
      switch ( *((_DWORD *)v1 + 4) ) /*0x180367407*/
      {
        case 0: /*0x180367407*/
        case 1: /*0x180367407*/
          *((_DWORD *)v1 + 4) = 2; /*0x180367409*/
          return; /*0x180367410*/
        case 3: /*0x180367407*/
          avs2_core_244(a1: v6 + 8, a2: 0, a3: 28); /*0x18036741f*/
          *(_BYTE *)(v6 + 64) = 1; /*0x180367425*/
          *(_BYTE *)(v6 + 66) = 1; /*0x180367429*/
          *((_DWORD *)v1 + 4) = 4; /*0x18036742d*/
          goto LABEL_14; /*0x18036742d*/
        case 4: /*0x180367407*/
LABEL_14:
          if ( *(_BYTE *)(v6 + 49) == 0 ) /*0x180367437*/
            return; /*0x180367437*/
          LOBYTE(v2) = *(_DWORD *)(v6 + 16) != 1; /*0x180367448*/
          *((_DWORD *)v1 + 16) = v2 + 1; /*0x18036744d*/
          sub_180365B00(a1: v6, a2: v1 + 3); /*0x180367450*/
          if ( *(_BYTE *)(sub_180311A00(a1: v7) + 36) == 0 ) /*0x18036745e*/
            goto LABEL_20; /*0x18036745e*/
          sub_180365B00(a1: v6, a2: v10); /*0x180367468*/
          if ( (unsigned int)sub_180001617(a1: v9, a2: v10) != 0 /*0x18036749a*/
            || (v8 = 2 - (*(_DWORD *)(v6 + 16) != 0), (unsigned int)sub_18000162F(a1: &v8) != 0) )
          {
            *(void **)((char *)v1 + 41) = nullptr; /*0x1803674c4*/
            *(void **)((char *)v1 + 49) = nullptr; /*0x1803674c8*/
            *((_BYTE *)v1 + 57) = 0; /*0x1803674cc*/
LABEL_20:
            *((_BYTE *)v1 + 21) = 1; /*0x1803674cf*/
            *((_DWORD *)v1 + 4) = 5; /*0x1803674d3*/
          }
          else
          {
            avs2_core_258(a1: (char *)v1 + 41, a2: 17, a3: &unk_18040AEE4); /*0x1803674af*/
            *((_BYTE *)v1 + 21) = 1; /*0x1803674b5*/
            *((_DWORD *)v1 + 4) = 5; /*0x1803674b9*/
          }
          break; /*0x1803674c0*/
        case 5: /*0x180367407*/
          *((_DWORD *)v1 + 4) = 6; /*0x1803674dc*/
          return; /*0x1803674e3*/
        case 6: /*0x180367407*/
          *(_BYTE *)(v6 + 64) = 0; /*0x1803674e5*/
          *(_BYTE *)(v6 + 67) = 1; /*0x1803674e8*/
          goto LABEL_23; /*0x1803674e8*/
        case 7: /*0x180367407*/
LABEL_23:
          *((_DWORD *)v1 + 4) = 1; /*0x1803674ec*/
          return; /*0x1803674ec*/
        default:
          return;
      }
    }
  }
}
