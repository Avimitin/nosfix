__int64 __fastcall sub_180365DA0(__int64 a1)
{
  __int64 v1; // rsi
  unsigned __int8 v2; // r15
  __int64 v3; // rcx
  char v4; // dl
  int v5; // ecx
  int v6; // edi
  unsigned int v7; // r14d
  unsigned int *v8; // rbx
  char is_active_device; // al
  char v10; // al
  int v11; // ebx
  char v12; // r14
  char v13; // cl
  int v14; // ebp
  __int64 v16; // [rsp+50h] [rbp+8h] BYREF

  v16 = a1; /*0x180365da0*/
  v1 = qword_1812D1E68; /*0x180365dac*/
  v2 = 0; /*0x180365db3*/
  switch ( *(_DWORD *)qword_1812D1E68 ) /*0x180365de7*/
  {
    case 0: /*0x180365de7*/
      *(_DWORD *)qword_1812D1E68 = *(_BYTE *)(qword_1812D1E68 + 36) != 0; /*0x180365df3*/
      return v2; /*0x180365df5*/
    case 1: /*0x180365de7*/
      *(_DWORD *)qword_1812D1E68 = 2; /*0x180365dff*/
      return v2; /*0x180365e01*/
    case 2: /*0x180365de7*/
      if ( *(_BYTE *)(sub_180311A00() + 36) == 0 /*0x180365e3b*/
        || (LOBYTE(v3) = 2,
            (unsigned __int8)ac_io_begin(a1: v3, a2: "1.30.5", a3: &v16, a4: 262146, a5: 0, a6: 115200) != 0) )
      {
        v4 = 1; /*0x180365e49*/
      }
      else
      {
        v4 = 0; /*0x180365e42*/
        *(_DWORD *)(v1 + 4) = 1; /*0x180365e44*/
      }
      v5 = 4; /*0x180365e4d*/
      if ( v4 == 0 ) /*0x180365e57*/
        v5 = 7; /*0x180365e57*/
      *(_DWORD *)v1 = v5; /*0x180365e5a*/
      return v2; /*0x180365e5c*/
    case 3: /*0x180365de7*/
      *(_DWORD *)v1 = sub_180366D50(a1: qword_1812D1E68); /*0x180365ec5*/
      return v2; /*0x180365ec7*/
    case 4: /*0x180365de7*/
      if ( *(_BYTE *)(sub_180311A00() + 37) == 0 /*0x180365e9b*/
        || (unsigned __int8)ac_io_begin(a1: 1, a2: "1.30.5", a3: &v16, a4: 1006632960, a5: 1, a6: 115200) != 0 )
      {
        *(_DWORD *)v1 = 3; /*0x180365eb6*/
      }
      else
      {
        *(_DWORD *)(v1 + 4) = 3; /*0x180365ea2*/
        *(_DWORD *)v1 = 7; /*0x180365eaa*/
      }
      return v2; /*0x180365eac*/
    case 5: /*0x180365de7*/
      v6 = 1; /*0x180365ed1*/
      if ( *(_BYTE *)(sub_180311A00() + 37) == 0 ) /*0x180365eda*/
      {
        *(_BYTE *)(v1 + 63) = 1; /*0x180365edc*/
        goto LABEL_35; /*0x180365ee0*/
      }
      if ( (unsigned __int8)ac_io_is_active2(a1: 4, a2: &v16, a3: 1) != 0 ) /*0x180365efc*/
      {
        v7 = 0; /*0x180365efe*/
        v8 = (unsigned int *)&unk_18070C410; /*0x180365f01*/
        while ( 1 ) /*0x180365f14*/
        {
          is_active_device = ac_io_is_active_device(a1: *v8, a2: 1); /*0x180365f14*/
          *((_BYTE *)v8 + 12) = is_active_device; /*0x180365f1a*/
          if ( is_active_device == 0 ) /*0x180365f1f*/
            break; /*0x180365f1f*/
LABEL_25:
          ++v7; /*0x180365f4d*/
          v8 += 4; /*0x180365f50*/
          if ( v7 >= 4 ) /*0x180365f58*/
          {
            *(_BYTE *)(v1 + 63) = 1; /*0x180365f5a*/
            goto LABEL_30; /*0x180365f5e*/
          }
        }
        while ( (int)v8[1] < 5 ) /*0x180365f25*/
        {
          if ( (int)++v8[2] > 60 ) /*0x180365f32*/
          {
            v10 = ac_io_is_active_device(a1: *v8, a2: 1); /*0x180365f38*/
            ++v8[1]; /*0x180365f3e*/
            *((_BYTE *)v8 + 12) = v10; /*0x180365f41*/
            v8[2] = 0; /*0x180365f44*/
          }
          if ( *((_BYTE *)v8 + 12) != 0 ) /*0x180365f4b*/
            goto LABEL_25; /*0x180365f4b*/
        }
        goto LABEL_33; /*0x180365f25*/
      }
      if ( (int)v16 >= 8 ) /*0x180365f65*/
      {
        if ( dword_1813EE5B8 >= 1 ) /*0x180365f6d*/
        {
LABEL_33:
          *(_DWORD *)(v1 + 4) = 3; /*0x180365fbd*/
          v6 = 2; /*0x180365fc5*/
          goto LABEL_35; /*0x180365fc8*/
        }
        ac_io_reset(a1: 1); /*0x180365f71*/
        ++dword_1813EE5B8; /*0x180365f77*/
      }
LABEL_30:
      if ( *(_BYTE *)(v1 + 63) != 0 ) /*0x180365f81*/
      {
        ac_io_set_soft_watch_dog(a1: 0, a2: 1); /*0x180365f87*/
        ac_io_begin_get_status(a1: 0x4000000, a2: 1); /*0x180365f94*/
        ac_io_panb_start_auto_input(); /*0x180365f9a*/
        dword_1813EE5BC = 0; /*0x180365fa0*/
        goto LABEL_35; /*0x180365fa6*/
      }
      if ( ++dword_1813EE5BC >= 3600 ) /*0x180365fbb*/
        goto LABEL_33; /*0x180365fbb*/
      v6 = 0; /*0x180365fca*/
LABEL_35:
      if ( v6 != 0 ) /*0x180365fce*/
      {
        if ( v6 == 1 ) /*0x180365fd3*/
          *(_DWORD *)v1 = 6; /*0x180365fe6*/
        else
          *(_DWORD *)v1 = 7; /*0x180365fda*/
      }
      else
      {
        *(_DWORD *)v1 = 5; /*0x180365ff2*/
      }
      return v2; /*0x1803660a4*/
    case 6: /*0x180365de7*/
      v11 = 7; /*0x180365ffc*/
      v12 = sub_180366310(a1: qword_1812D1E68); /*0x180366006*/
      if ( *(_BYTE *)(sub_180311A00() + 37) != 0 ) /*0x180366013*/
      {
        if ( (unsigned __int8)ac_io_is_active_device(a1: 26, a2: 1) != 0 /*0x180366035*/
          && (unsigned __int8)ac_io_panb_update_control_status_buffer() != 0 )
        {
          ac_io_panb_get_control_status_buffer(a1: v1 + 2224); /*0x18036603e*/
          v14 = 0; /*0x180366044*/
        }
        else
        {
          v14 = dword_1813EE5C0 + 1; /*0x18036604e*/
        }
        dword_1813EE5C0 = v14; /*0x180366050*/
        if ( v14 <= 600 ) /*0x18036605c*/
        {
          ac_io_update(a1: 1); /*0x18036606c*/
          v13 = 1; /*0x180366072*/
        }
        else
        {
          v13 = 0; /*0x180366063*/
          *(_DWORD *)(v1 + 4) = 5; /*0x180366065*/
        }
      }
      else
      {
        v13 = 1; /*0x180366015*/
      }
      if ( v12 != 0 && v13 != 0 ) /*0x180366082*/
        v11 = 6; /*0x180366082*/
      *(_DWORD *)v1 = v11; /*0x180366085*/
      return v2; /*0x180366087*/
    case 7: /*0x180365de7*/
      return 1; /*0x180366089*/
    default:
      return v2;
  }
}
