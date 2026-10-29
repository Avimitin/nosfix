int __fastcall sub_1801C3FE0(__int64 a1)
{
  __int64 v2; // rax
  __int64 v3; // rbx
  _OWORD *v4; // rdx
  const char *v5; // rax
  double v6; // xmm1_8
  int v7; // ecx
  __int64 v8; // rdx
  __int64 v9; // rbx
  __int64 v10; // rdi
  __int64 v11; // r8
  __int64 v12; // rdx
  __int64 v13; // rcx
  __int64 v14; // rbx
  __int64 v15; // rdx
  int *v16; // rbx
  __int64 v17; // rcx
  __int64 v18; // rdx
  int v20; // eax
  int v21; // eax
  __int64 v22; // rdx
  int v23; // eax
  double v24; // xmm6_8
  int v25; // edi
  const char *v26; // rbx
  CLibXact *Instance; // rax
  DWORD SoundLabelLapse; // ecx
  CLibXact *v29; // rax
  unsigned int v30; // eax
  _QWORD **v31; // r14
  _QWORD *v32; // rbx
  _BYTE *v33; // rdi
  __int64 (__fastcall *v34)(); // rdx
  char v35; // al
  __int64 v36; // r15
  _DWORD *v37; // rsi
  int v38; // ecx
  int v39; // ecx
  int v40; // eax
  _QWORD *i; // rbx
  _BYTE *v42; // rdi
  __int64 (__fastcall *v43)(); // rdx
  char v44; // al
  __int64 v45; // rdx
  __int64 v46; // rdx
  __int64 v47; // rcx
  __int64 v48; // rdx
  __int64 v49; // rcx
  __int64 v50; // rdx
  __int64 v51; // rax
  LARGE_INTEGER PerformanceCount; // [rsp+38h] [rbp-D0h] BYREF
  struct tagMSG PerformanceCount_8; // [rsp+40h] [rbp-C8h] BYREF
  __int64 v55; // [rsp+70h] [rbp-98h] BYREF
  __int64 v56; // [rsp+78h] [rbp-90h]
  struct tagMSG Msg; // [rsp+80h] [rbp-88h] BYREF
  __int64 v58; // [rsp+B0h] [rbp-58h]
  __int128 v59; // [rsp+B8h] [rbp-50h] BYREF
  __int128 v60; // [rsp+C8h] [rbp-40h] BYREF

  v58 = -2; /*0x1801c3ff7*/
  memset(&Msg, 0, sizeof(Msg)); /*0x1801c4030*/
  v2 = sub_1802C5260(); /*0x1801c4049*/
  v3 = v2 + 16; /*0x1801c404e*/
  *(_QWORD *)(v2 + 24) = *(_QWORD *)(v2 + 16); /*0x1801c4055*/
  PerformanceCount.QuadPart = 0; /*0x1801c405c*/
  QueryPerformanceCounter(lpPerformanceCount: &PerformanceCount); /*0x1801c4066*/
  unknown_libname_4(a1: &v60); /*0x1801c4070*/
  *(LARGE_INTEGER *)&v59 = PerformanceCount; /*0x1801c407a*/
  *((_QWORD *)&v59 + 1) = "(no-label)"; /*0x1801c4085*/
  v60 = xmmword_1812D3E10; /*0x1801c4090*/
  v4 = *(_OWORD **)(v3 + 8); /*0x1801c4094*/
  if ( *(_OWORD **)(v3 + 16) == v4 ) /*0x1801c409c*/
  {
    sub_1800C7E20(a1: v3, a2: v4, a3: &v59); /*0x1801c40bb*/
  }
  else
  {
    *v4 = v59; /*0x1801c40a2*/
    v4[1] = v60; /*0x1801c40a9*/
    *(_QWORD *)(v3 + 8) += 32LL; /*0x1801c40ad*/
  }
  LODWORD(v5) = timeGetTime() + 16; /*0x1801c40c6*/
  dword_1813E94E0 = (int)v5; /*0x1801c40c9*/
  if ( *(_BYTE *)(a1 + 8) == 0 )
  {
    while ( 1 )
    {
      PerformanceCount.QuadPart = 0; /*0x1801c4120*/
      QueryPerformanceCounter(lpPerformanceCount: &PerformanceCount); /*0x1801c412a*/
      v6 = (double)(PerformanceCount.LowPart - (int)qword_1813E6D98) / (double)(int)Frequency.LowPart; /*0x1801c4153*/
      qword_1813E6D98 = PerformanceCount.QuadPart; /*0x1801c4157*/
      v7 = dword_1813E6C28; /*0x1801c415e*/
      dbl_1812CF790[dword_1813E6C28] = v6; /*0x1801c416c*/
      qword_1813E7FE8 = *(_QWORD *)&v6; /*0x1801c4171*/
      dword_1813E6C28 = (v7 + 1) % 30; /*0x1801c4193*/
      if ( (dword_184ACF5E8 & 1) == 0 ) /*0x1801c41a1*/
      {
        dword_184ACF5E8 |= 1u; /*0x1801c41a6*/
        qword_184ACF5F0 = (__int64)&CPerformanceCounter::`vftable'; /*0x1801c41ac*/
        word_184ACF5F8 = 0; /*0x1801c41b3*/
        dword_184ACF5FC = 0; /*0x1801c41bc*/
        xmmword_184ACF600 = 0; /*0x1801c41c6*/
        xmmword_184ACF610 = 0; /*0x1801c41d1*/
        xmmword_184ACF620 = 0; /*0x1801c41d9*/
        atexit(a1: sub_1803A16D0); /*0x1801c41e8*/
      }
      PerformanceCount.QuadPart = 0; /*0x1801c41ed*/
      QueryPerformanceCounter(lpPerformanceCount: &PerformanceCount); /*0x1801c41f7*/
      unknown_libname_4(a1: &PerformanceCount_8.wParam); /*0x1801c4202*/
      PerformanceCount_8.hwnd = (HWND)PerformanceCount.QuadPart; /*0x1801c420c*/
      *(_QWORD *)&PerformanceCount_8.message = "SYSTEM2"; /*0x1801c4211*/
      *(_OWORD *)&PerformanceCount_8.wParam = xmmword_1812D3E20; /*0x1801c421d*/
      v8 = *((_QWORD *)&xmmword_184ACF600 + 1); /*0x1801c4222*/
      if ( (_QWORD)xmmword_184ACF610 == *((_QWORD *)&xmmword_184ACF600 + 1) ) /*0x1801c4230*/
      {
        sub_1800C7E20(a1: &xmmword_184ACF600, a2: *((_QWORD *)&xmmword_184ACF600 + 1), a3: &PerformanceCount_8); /*0x1801c4259*/
      }
      else
      {
        **((_OWORD **)&xmmword_184ACF600 + 1) = *(_OWORD *)&PerformanceCount_8.hwnd; /*0x1801c4237*/
        *(_OWORD *)(v8 + 16) = *(_OWORD *)&PerformanceCount_8.wParam; /*0x1801c423f*/
        *((_QWORD *)&xmmword_184ACF600 + 1) += 32LL; /*0x1801c4243*/
      }
      v9 = qword_1812D1EA8; /*0x1801c425e*/
      if ( qword_1812D1EA8 != 0 ) /*0x1801c4268*/
      {
        v10 = *(_QWORD *)(qword_1812D1EA8 + 16616); /*0x1801c426e*/
        while ( v10 != 0 ) /*0x1801c4278*/
        {
          v11 = v10; /*0x1801c4280*/
          v12 = v10 + 8; /*0x1801c4283*/
          v10 = *(_QWORD *)(v10 + 8); /*0x1801c4287*/
          if ( *(_BYTE *)(*(_QWORD *)v11 + 16LL) != 0 ) /*0x1801c4291*/
          {
            if ( *(_QWORD *)(v11 + 16) == 0 ) /*0x1801c4298*/
            {
              if ( v10 != 0 ) /*0x1801c429d*/
                *(_QWORD *)(v9 + 16616) = *(_QWORD *)(*(_QWORD *)(v9 + 16616) + 8LL); /*0x1801c42b3*/
              else
                *(_QWORD *)(v9 + 16616) = 0; /*0x1801c429f*/
            }
            if ( *(_QWORD *)v12 != 0 ) /*0x1801c42c0*/
              *(_QWORD *)(*(_QWORD *)v12 + 16LL) = *(_QWORD *)(v11 + 16); /*0x1801c42c6*/
            v13 = *(_QWORD *)(v11 + 16); /*0x1801c42ca*/
            if ( v13 != 0 ) /*0x1801c42d1*/
              *(_QWORD *)(v13 + 8) = *(_QWORD *)v12; /*0x1801c42d6*/
            *(_QWORD *)v12 = *(_QWORD *)(v9 + 16608); /*0x1801c42e1*/
            *(_QWORD *)(v9 + 16608) = v11; /*0x1801c42e4*/
            --*(_DWORD *)(v9 + 16624); /*0x1801c42eb*/
            j_j_free(Block: *(void **)v11); /*0x1801c42f9*/
          }
        }
        sub_180102B80(a1: v9 + 64, a2: 0, a3: 0x4000); /*0x1801c4313*/
        *(_DWORD *)(v9 + 16584) = 0; /*0x1801c4318*/
        *(_BYTE *)(v9 + 16590) = 1; /*0x1801c431f*/
      }
      for ( LODWORD(v5) = PeekMessageA(lpMsg: &Msg, hWnd: nullptr, wMsgFilterMin: 0, wMsgFilterMax: 0, wRemoveMsg: 1u); /*0x1801c434a*/
            (_DWORD)v5 != 0;
            LODWORD(v5) = PeekMessageA(lpMsg: &Msg, hWnd: nullptr, wMsgFilterMin: 0, wMsgFilterMax: 0, wRemoveMsg: 1u) )
      {
        TranslateMessage(lpMsg: &Msg); /*0x1801c4355*/
        DispatchMessageA(lpMsg: &Msg); /*0x1801c4360*/
      }
      if ( *(_BYTE *)(a1 + 8) != 0 ) /*0x1801c438a*/
        return (int)v5; /*0x1801c438a*/
      if ( qword_1812D1EA8 != 0 && byte_180701AA4 != 0 ) /*0x1801c43a1*/
        sub_180353E70(a1: &off_180701A80); /*0x1801c43aa*/
      if ( (unsigned __int8)sub_180363060() != 0 )
      {
        dword_1813E960C = 0; /*0x1801c43b8*/
      }
      else if ( ++dword_1813E960C > 4200 )
      {
        v14 = avs2_core_44(a1: 1); /*0x1801c43e8*/
        avs2_core_8(a1: 0); /*0x1801c43ed*/
        if ( (unsigned __int8)sub_180363060() == 0 )
        {
          while ( (unsigned __int64)(avs2_core_44(a1: 1) - v14) <= 0x4E20 ) /*0x1801c4414*/
          {
            avs2_core_8(a1: 0); /*0x1801c4418*/
            if ( (unsigned __int8)sub_180363060() != 0 ) /*0x1801c4425*/
              goto LABEL_39; /*0x1801c4425*/
          }
          if ( byte_180707494 != 0 ) /*0x1801c4430*/
          {
LABEL_132:
            sub_1800D64B0(); /*0x1801c4cf3*/
            sub_1800D62F0(); /*0x1801c4cf8*/
            v51 = sub_180360060(); /*0x1801c4cfd*/
            sub_18035FE90(a1: v51); /*0x1801c4d05*/
            memset(&PerformanceCount_8, 0, sizeof(PerformanceCount_8)); /*0x1801c4d0c*/
            while ( 1 ) /*0x1801c4d33*/
            {
              dword_1813E94E0 = timeGetTime() + 16; /*0x1801c4d33*/
              ++dword_180707490; /*0x1801c4d39*/
              if ( PeekMessageA( /*0x1801c4d54*/
                     lpMsg: &PerformanceCount_8,
                     hWnd: nullptr,
                     wMsgFilterMin: 0,
                     wMsgFilterMax: 0,
                     wRemoveMsg: 1u) )
              {
                TranslateMessage(lpMsg: &PerformanceCount_8); /*0x1801c4d63*/
                DispatchMessageA(lpMsg: &PerformanceCount_8); /*0x1801c4d6e*/
              }
              if ( *((_BYTE *)qword_1811CEEB0 + 2769) != 0 ) /*0x1801c4d82*/
                sub_1801CA8A0(a1: &dword_1812D3428); /*0x1801c4d8b*/
              sub_18036A640(); /*0x1801c4d90*/
              sub_1800D63F0(); /*0x1801c4d98*/
              sub_1802BA230(a1: &dword_180707490); /*0x1801c4da4*/
              sub_1802BA450(); /*0x1801c4da9*/
            }
          }
          byte_180707494 = 1; /*0x1801c4436*/
          qword_180707498 = 0; /*0x1801c443d*/
          avs2_core_380(a1: "ErrorManager", a2: "%s, ERROR CODE : %s", "HDD ERROR", "5-1501-0000");
        }
      }
LABEL_39:
      if ( byte_180707494 != 0 ) /*0x1801c4471*/
        goto LABEL_132; /*0x1801c4471*/
      if ( ++*(_DWORD *)(a1 + 208) == 60 ) /*0x1801c4486*/
      {
        *(_DWORD *)(a1 + 208) = 0; /*0x1801c4488*/
        ShowCursor(bShow: false); /*0x1801c4491*/
      }
      if ( (dword_184ACF5E8 & 1) == 0 ) /*0x1801c449f*/
      {
        dword_184ACF5E8 |= 1u; /*0x1801c44a4*/
        qword_184ACF5F0 = (__int64)&CPerformanceCounter::`vftable'; /*0x1801c44aa*/
        word_184ACF5F8 = 0; /*0x1801c44b1*/
        dword_184ACF5FC = 0; /*0x1801c44ba*/
        xmmword_184ACF600 = 0; /*0x1801c44c4*/
        xmmword_184ACF610 = 0; /*0x1801c44cf*/
        xmmword_184ACF620 = 0; /*0x1801c44d7*/
        atexit(a1: sub_1803A16D0); /*0x1801c44e6*/
      }
      PerformanceCount.QuadPart = 0; /*0x1801c44eb*/
      QueryPerformanceCounter(lpPerformanceCount: &PerformanceCount); /*0x1801c44f5*/
      unknown_libname_4(a1: &PerformanceCount_8.wParam); /*0x1801c4500*/
      PerformanceCount_8.hwnd = (HWND)PerformanceCount.QuadPart; /*0x1801c450a*/
      *(_QWORD *)&PerformanceCount_8.message = "NETWORK"; /*0x1801c450f*/
      *(_OWORD *)&PerformanceCount_8.wParam = xmmword_1812D3E30; /*0x1801c451b*/
      v15 = *((_QWORD *)&xmmword_184ACF600 + 1); /*0x1801c4520*/
      if ( (_QWORD)xmmword_184ACF610 == *((_QWORD *)&xmmword_184ACF600 + 1) ) /*0x1801c452e*/
      {
        sub_1800C7E20(a1: &xmmword_184ACF600, a2: *((_QWORD *)&xmmword_184ACF600 + 1), a3: &PerformanceCount_8); /*0x1801c4557*/
      }
      else
      {
        **((_OWORD **)&xmmword_184ACF600 + 1) = *(_OWORD *)&PerformanceCount_8.hwnd; /*0x1801c4535*/
        *(_OWORD *)(v15 + 16) = *(_OWORD *)&PerformanceCount_8.wParam; /*0x1801c453d*/
        *((_QWORD *)&xmmword_184ACF600 + 1) += 32LL; /*0x1801c4541*/
      }
      if ( *((_BYTE *)qword_1811CEEB0 + 2769) != 0 ) /*0x1801c456a*/
        sub_1801CA8A0(a1: &dword_1812D3428); /*0x1801c4573*/
      if ( (dword_1813EC2F8 & 1) == 0 ) /*0x1801c4580*/
      {
        dword_1813EC2F8 |= 1u; /*0x1801c4585*/
        __wind /*0xf1c000000000001c*/
        {
          qword_1813EC150 = (__int64)&CXrpcCtrl::`vftable'; /*0x1801c4592*/
          byte_1813EC158 = 0; /*0x1801c4599*/
          `eh vector constructor iterator'(a1: &unk_1813EC160, a2: 0x18u, a3: 0x11u, a4: sub_1801D84C0, a5: nullsub_21); /*0x1801c45c3*/
          atexit(a1: sub_180394CE0); /*0x1801c45cf*/
        }
        __unwind
        {
          dword_1813EC2F8 &= ~1u; /*0x180382f5e*/
        }
      }
      if ( byte_1813EC158 != 0 ) /*0x1801c45dc*/
        break; /*0x1801c45dc*/
LABEL_68:
      if ( (dword_184ACF5E8 & 1) == 0 ) /*0x1801c4694*/
      {
        dword_184ACF5E8 |= 1u; /*0x1801c4699*/
        qword_184ACF5F0 = (__int64)&CPerformanceCounter::`vftable'; /*0x1801c469f*/
        word_184ACF5F8 = 0; /*0x1801c46a6*/
        dword_184ACF5FC = 0; /*0x1801c46af*/
        xmmword_184ACF600 = 0; /*0x1801c46b9*/
        xmmword_184ACF610 = 0; /*0x1801c46c4*/
        xmmword_184ACF620 = 0; /*0x1801c46cc*/
        atexit(a1: sub_1803A16D0); /*0x1801c46db*/
      }
      PerformanceCount.QuadPart = 0; /*0x1801c46e0*/
      QueryPerformanceCounter(lpPerformanceCount: &PerformanceCount); /*0x1801c46ea*/
      unknown_libname_4(a1: &PerformanceCount_8.wParam); /*0x1801c46f5*/
      PerformanceCount_8.hwnd = (HWND)PerformanceCount.QuadPart; /*0x1801c46ff*/
      *(_QWORD *)&PerformanceCount_8.message = "IO"; /*0x1801c470b*/
      *(_OWORD *)&PerformanceCount_8.wParam = xmmword_1812D3E20; /*0x1801c4717*/
      v22 = *((_QWORD *)&xmmword_184ACF600 + 1); /*0x1801c471c*/
      if ( (_QWORD)xmmword_184ACF610 == *((_QWORD *)&xmmword_184ACF600 + 1) ) /*0x1801c472a*/
      {
        sub_1800C7E20(a1: &xmmword_184ACF600, a2: *((_QWORD *)&xmmword_184ACF600 + 1), a3: &PerformanceCount_8); /*0x1801c4775*/
      }
      else
      {
        **((_OWORD **)&xmmword_184ACF600 + 1) = *(_OWORD *)&PerformanceCount_8.hwnd; /*0x1801c4731*/
        *(_OWORD *)(v22 + 16) = *(_OWORD *)&PerformanceCount_8.wParam; /*0x1801c4739*/
        *((_QWORD *)&xmmword_184ACF600 + 1) += 32LL; /*0x1801c473d*/
      }
      sub_18036A640(); /*0x1801c477a*/
      v24 = *(double *)&qword_18070B2F8; /*0x1801c477f*/
      if ( dword_18070A610 > 0 ) /*0x1801c478f*/
      {
        if ( dword_18070A750 < 1 ) /*0x1801c47a6*/
        {
          v25 = (int)((double)dword_18070A610 * 1000.0 / *(double *)&qword_18070B2F8); /*0x1801c47d4*/
          v26 = (const char *)(sub_180360060() + 680); /*0x1801c47de*/
          Instance = (CLibXact *)CLibXact::getInstance(); /*0x1801c47e5*/
          if ( (int)CLibXact::GetSoundLabelLapse(this: Instance, a2: v26) > 0 ) /*0x1801c47fd*/
          {
            sub_180360060(); /*0x1801c480f*/
            v29 = (CLibXact *)CLibXact::getInstance(); /*0x1801c4814*/
            SoundLabelLapse = (int)CLibXact::GetSoundLabelLapse(this: v29, a2: (const char *)word_18070A618); /*0x1801c482a*/
          }
          else
          {
            SoundLabelLapse = timeGetTime() - HIDWORD(qword_18070A718); /*0x1801c4807*/
          }
          if ( (int)qword_18070A740 <= 0 ) /*0x1801c4836*/
          {
            if ( (int)qword_18070A740 < 0 ) /*0x1801c4852*/
            {
              v30 = (int)((double)(int)qword_18070A740 * -1.0 * 1000.0 / v24); /*0x1801c486a*/
              if ( SoundLabelLapse >= v30 ) /*0x1801c4871*/
                SoundLabelLapse -= v30; /*0x1801c4878*/
              else
                SoundLabelLapse = 0; /*0x1801c4873*/
            }
          }
          else
          {
            SoundLabelLapse += (int)((double)(int)qword_18070A740 * 1000.0 / v24); /*0x1801c484e*/
          }
          LODWORD(qword_18070A718) = SoundLabelLapse - v25; /*0x1801c487c*/
        }
        else
        {
          LODWORD(qword_18070A718) = 16 * dword_18070A750-- + 10; /*0x1801c47b0*/
        }
      }
      else
      {
        LODWORD(qword_18070A718) = 0; /*0x1801c4791*/
      }
      v31 = (_QWORD **)qword_1812D1EB8; /*0x1801c4882*/
      if ( qword_1812D1EB8 != nullptr )
      {
        v32 = **(_QWORD ***)qword_1812D1EB8; /*0x1801c4895*/
        if ( v32 != *(_QWORD **)qword_1812D1EB8 ) /*0x1801c489b*/
        {
          do /*0x1801c48d1*/
          {
            v33 = (_BYTE *)v32[2]; /*0x1801c48a0*/
            v34 = *(__int64 (__fastcall **)())(*(_QWORD *)v33 + 16LL); /*0x1801c48a7*/
            if ( v34 == sub_180370030 ) /*0x1801c48ae*/
              v35 = v33[8]; /*0x1801c48b4*/
            else
              v35 = ((__int64 (__fastcall *)(_QWORD))v34)(a1: v32[2]); /*0x1801c4dc0*/
            if ( v35 != 0 && v33[9] != 0 ) /*0x1801c48c0*/
              (*(void (__fastcall **)(_BYTE *))(*(_QWORD *)v33 + 24LL))(a1: v33); /*0x1801c48c8*/
            v32 = (_QWORD *)*v32; /*0x1801c48cb*/
          }
          while ( v32 != *v31 ); /*0x1801c48d1*/
        }
        if ( qword_1812D1EA8 != 0 )
        {
          v36 = *(int *)(qword_1812D1EA8 + 16584); /*0x1801c48e3*/
          if ( v36 > 0 )
          {
            v37 = (_DWORD *)(qword_1812D1EA8 + 72); /*0x1801c48f3*/
            do
            {
              if ( v37 != (_DWORD *)8 )
              {
                v38 = v37[1]; /*0x1801c490d*/
                if ( v38 != 0 )
                {
                  v39 = v38 - 1; /*0x1801c4914*/
                  if ( v39 != 0 )
                    v40 = v39 == 2 ? 4 : 1;
                  else
                    v40 = 2; /*0x1801c4931*/
                }
                else
                {
                  v40 = 0; /*0x1801c4938*/
                }
                LODWORD(v59) = v40; /*0x1801c493b*/
                DWORD1(v59) = *v37; /*0x1801c4940*/
                *((_QWORD *)&v59 + 1) = *((_QWORD *)v37 - 1); /*0x1801c4945*/
                for ( i = (_QWORD *)**v31; i != *v31; i = (_QWORD *)*i ) /*0x1801c4957*/
                {
                  v42 = (_BYTE *)i[2]; /*0x1801c4960*/
                  v43 = *(__int64 (__fastcall **)())(*(_QWORD *)v42 + 16LL); /*0x1801c4967*/
                  if ( v43 == sub_180370030 ) /*0x1801c496e*/
                    v44 = v42[8]; /*0x1801c4974*/
                  else
                    v44 = ((__int64 (__fastcall *)(_QWORD))v43)(a1: i[2]); /*0x1801c4db6*/
                  if ( v44 != 0 ) /*0x1801c497a*/
                    (*(void (__fastcall **)(_BYTE *, __int128 *))(*(_QWORD *)v42 + 32LL))(a1: v42, a2: &v59); /*0x1801c4986*/
                }
              }
              v37 += 4; /*0x1801c4994*/
              --v36; /*0x1801c4998*/
            }
            while ( v36 != 0 );
          }
        }
      }
      if ( (dword_184ACF5E8 & 1) == 0 ) /*0x1801c49b8*/
      {
        dword_184ACF5E8 |= 1u; /*0x1801c49bd*/
        qword_184ACF5F0 = (__int64)&CPerformanceCounter::`vftable'; /*0x1801c49c3*/
        word_184ACF5F8 = 0; /*0x1801c49ca*/
        dword_184ACF5FC = 0; /*0x1801c49d3*/
        xmmword_184ACF600 = 0; /*0x1801c49dd*/
        xmmword_184ACF610 = 0; /*0x1801c49e8*/
        xmmword_184ACF620 = 0; /*0x1801c49f0*/
        atexit(a1: sub_1803A16D0); /*0x1801c49ff*/
      }
      PerformanceCount.QuadPart = 0; /*0x1801c4a04*/
      QueryPerformanceCounter(lpPerformanceCount: &PerformanceCount); /*0x1801c4a0e*/
      unknown_libname_4(a1: &PerformanceCount_8.wParam); /*0x1801c4a19*/
      PerformanceCount_8.hwnd = (HWND)PerformanceCount.QuadPart; /*0x1801c4a23*/
      *(_QWORD *)&PerformanceCount_8.message = "BM2D"; /*0x1801c4a2f*/
      *(_OWORD *)&PerformanceCount_8.wParam = xmmword_1812D3E40; /*0x1801c4a3b*/
      v45 = *((_QWORD *)&xmmword_184ACF600 + 1); /*0x1801c4a40*/
      if ( (_QWORD)xmmword_184ACF610 == *((_QWORD *)&xmmword_184ACF600 + 1) ) /*0x1801c4a4e*/
      {
        sub_1800C7E20(a1: &xmmword_184ACF600, a2: *((_QWORD *)&xmmword_184ACF600 + 1), a3: &PerformanceCount_8); /*0x1801c4a77*/
      }
      else
      {
        **((_OWORD **)&xmmword_184ACF600 + 1) = *(_OWORD *)&PerformanceCount_8.hwnd; /*0x1801c4a55*/
        *(_OWORD *)(v45 + 16) = *(_OWORD *)&PerformanceCount_8.wParam; /*0x1801c4a5d*/
        *((_QWORD *)&xmmword_184ACF600 + 1) += 32LL; /*0x1801c4a61*/
      }
      sub_1800D63F0(); /*0x1801c4a7f*/
      sub_1801C3B20(a1); /*0x1801c4a87*/
      sub_1801C31C0(a1); /*0x1801c4a8f*/
      dword_1813E94E0 = timeGetTime() + 16; /*0x1801c4a9d*/
      if ( (dword_184ACF5E8 & 1) == 0 ) /*0x1801c4aab*/
      {
        dword_184ACF5E8 |= 1u; /*0x1801c4ab0*/
        qword_184ACF5F0 = (__int64)&CPerformanceCounter::`vftable'; /*0x1801c4ab6*/
        word_184ACF5F8 = 0; /*0x1801c4abd*/
        dword_184ACF5FC = 0; /*0x1801c4ac6*/
        xmmword_184ACF600 = 0; /*0x1801c4ad0*/
        xmmword_184ACF610 = 0; /*0x1801c4adb*/
        xmmword_184ACF620 = 0; /*0x1801c4ae3*/
        atexit(a1: sub_1803A16D0); /*0x1801c4af2*/
      }
      PerformanceCount.QuadPart = 0; /*0x1801c4af7*/
      QueryPerformanceCounter(lpPerformanceCount: &PerformanceCount); /*0x1801c4b01*/
      unknown_libname_4(a1: &PerformanceCount_8.wParam); /*0x1801c4b0c*/
      PerformanceCount_8.hwnd = (HWND)PerformanceCount.QuadPart; /*0x1801c4b16*/
      *(_QWORD *)&PerformanceCount_8.message = &unk_1803D291D; /*0x1801c4b22*/
      *(_OWORD *)&PerformanceCount_8.wParam = xmmword_1813655B0; /*0x1801c4b2e*/
      v46 = *((_QWORD *)&xmmword_184ACF600 + 1); /*0x1801c4b33*/
      if ( (_QWORD)xmmword_184ACF610 == *((_QWORD *)&xmmword_184ACF600 + 1) ) /*0x1801c4b41*/
      {
        sub_1800C7E20(a1: &xmmword_184ACF600, a2: *((_QWORD *)&xmmword_184ACF600 + 1), a3: &PerformanceCount_8); /*0x1801c4b6d*/
        v47 = *((_QWORD *)&xmmword_184ACF600 + 1); /*0x1801c4b72*/
      }
      else
      {
        **((_OWORD **)&xmmword_184ACF600 + 1) = *(_OWORD *)&PerformanceCount_8.hwnd; /*0x1801c4b48*/
        *(_OWORD *)(v46 + 16) = *(_OWORD *)&PerformanceCount_8.wParam; /*0x1801c4b50*/
        v47 = *((_QWORD *)&xmmword_184ACF600 + 1) + 32LL; /*0x1801c4b5b*/
      }
      v48 = *((_QWORD *)&xmmword_184ACF610 + 1); /*0x1801c4b79*/
      *((_QWORD *)&xmmword_184ACF610 + 1) = xmmword_184ACF600; /*0x1801c4b87*/
      *(_QWORD *)&xmmword_184ACF600 = v48; /*0x1801c4b8e*/
      *(_QWORD *)&xmmword_184ACF620 = v47; /*0x1801c4b95*/
      v49 = *((_QWORD *)&xmmword_184ACF620 + 1); /*0x1801c4b9c*/
      *((_QWORD *)&xmmword_184ACF620 + 1) = xmmword_184ACF610; /*0x1801c4baa*/
      *(_QWORD *)&xmmword_184ACF610 = v49; /*0x1801c4bb1*/
      if ( (dword_184ACF5E8 & 1) == 0 ) /*0x1801c4bc0*/
      {
        dword_184ACF5E8 |= 1u; /*0x1801c4bc5*/
        qword_184ACF5F0 = (__int64)&CPerformanceCounter::`vftable'; /*0x1801c4bcb*/
        word_184ACF5F8 = 0; /*0x1801c4bd2*/
        dword_184ACF5FC = 0; /*0x1801c4bdb*/
        xmmword_184ACF600 = 0; /*0x1801c4be5*/
        xmmword_184ACF610 = 0; /*0x1801c4bf0*/
        xmmword_184ACF620 = 0; /*0x1801c4bf8*/
        atexit(a1: sub_1803A16D0); /*0x1801c4c07*/
        v48 = xmmword_184ACF600; /*0x1801c4c0c*/
      }
      *((_QWORD *)&xmmword_184ACF600 + 1) = v48; /*0x1801c4c13*/
      PerformanceCount.QuadPart = 0; /*0x1801c4c1a*/
      QueryPerformanceCounter(lpPerformanceCount: &PerformanceCount); /*0x1801c4c24*/
      unknown_libname_4(a1: &PerformanceCount_8.wParam); /*0x1801c4c2f*/
      PerformanceCount_8.hwnd = (HWND)PerformanceCount.QuadPart; /*0x1801c4c39*/
      v5 = "SYSTEM"; /*0x1801c4c3e*/
      *(_QWORD *)&PerformanceCount_8.message = "SYSTEM"; /*0x1801c4c45*/
      *(_OWORD *)&PerformanceCount_8.wParam = xmmword_1812D3E10; /*0x1801c4c51*/
      v50 = *((_QWORD *)&xmmword_184ACF600 + 1); /*0x1801c4c56*/
      if ( (_QWORD)xmmword_184ACF610 == *((_QWORD *)&xmmword_184ACF600 + 1) ) /*0x1801c4c64*/
      {
        LODWORD(v5) = sub_1800C7E20( /*0x1801c4c8d*/
                        a1: &xmmword_184ACF600,
                        a2: *((_QWORD *)&xmmword_184ACF600 + 1),
                        a3: &PerformanceCount_8);
      }
      else
      {
        **((_OWORD **)&xmmword_184ACF600 + 1) = *(_OWORD *)&PerformanceCount_8.hwnd; /*0x1801c4c6b*/
        *(_OWORD *)(v50 + 16) = *(_OWORD *)&PerformanceCount_8.wParam; /*0x1801c4c73*/
        *((_QWORD *)&xmmword_184ACF600 + 1) += 32LL; /*0x1801c4c77*/
      }
      if ( *(_BYTE *)(a1 + 8) != 0 ) /*0x1801c4cac*/
        return (int)v5; /*0x1801c4cac*/
    }
    v16 = (int *)&unk_1813EC168; /*0x1801c45e2*/
    while ( 1 ) /*0x1801c45f0*/
    {
      if ( v16 == (int *)8 ) /*0x1801c45f7*/
        goto LABEL_67; /*0x1801c45f7*/
      v17 = *((_QWORD *)v16 - 1); /*0x1801c45fd*/
      if ( v17 == 0 ) /*0x1801c4603*/
        goto LABEL_67; /*0x1801c4603*/
      v18 = (unsigned int)(v16[2] - 1); /*0x1801c4608*/
      if ( v16[2] == 1 ) /*0x1801c460b*/
      {
        v55 = *(_QWORD *)(v17 + 528); /*0x1801c4646*/
        LOWORD(v56) = *(_WORD *)(v17 + 536); /*0x1801c4653*/
        if ( (_BYTE)v56 == 0 ) /*0x1801c465a*/
        {
          v21 = sub_1801D88E0(a1: &v55, a2: v18); /*0x1801c4661*/
          if ( v21 < 0 ) /*0x1801c4668*/
            goto LABEL_66; /*0x1801c4668*/
          if ( v21 <= 2 ) /*0x1801c466d*/
          {
            v23 = *v16; /*0x1801c4747*/
            if ( *v16 < 3 ) /*0x1801c474c*/
            {
              v16[1] = 180; /*0x1801c4752*/
              v16[2] = 2; /*0x1801c4759*/
              *v16 = v23 + 1; /*0x1801c4762*/
              goto LABEL_67; /*0x1801c4764*/
            }
LABEL_66:
            v16[2] = 3; /*0x1801c4678*/
            goto LABEL_67; /*0x1801c4678*/
          }
          if ( v21 != 3 ) /*0x1801c4676*/
            goto LABEL_66; /*0x1801c4676*/
        }
      }
      else
      {
        if ( v16[2] != 2 ) /*0x1801c4610*/
          goto LABEL_67; /*0x1801c4610*/
        if ( --v16[1] >= 0 ) /*0x1801c4612*/
          goto LABEL_67; /*0x1801c4615*/
        if ( (*(unsigned __int8 (__fastcall **)(__int64))(*(_QWORD *)v17 + 88LL))(a1: v17) != 0 ) /*0x1801c461f*/
        {
          v16[2] = 1; /*0x1801c4621*/
        }
        else
        {
          v20 = *v16; /*0x1801c462a*/
          if ( *v16 >= 3 ) /*0x1801c462f*/
            goto LABEL_66; /*0x1801c462f*/
          v16[1] = 180; /*0x1801c4631*/
          *v16 = v20 + 1; /*0x1801c463a*/
        }
      }
LABEL_67:
      v16 += 6; /*0x1801c467f*/
      if ( (__int64)v16 >= (__int64)&dword_1813EC300 ) /*0x1801c4686*/
        goto LABEL_68; /*0x1801c4686*/
    }
  }
  return (int)v5; /*0x1801c4cb2*/
}
