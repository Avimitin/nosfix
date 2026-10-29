__int64 __fastcall sub_1801C31C0(__int64 a1)
{
  __int64 v2; // rax
  __int64 v3; // rbx
  _OWORD *v4; // rdx
  __int64 v5; // rax
  __int64 v6; // rbx
  _OWORD *v7; // rdx
  __int64 v8; // rax
  _QWORD *v9; // rbx
  __int64 v10; // rdi
  __int64 v11; // rdx
  __int64 v12; // rdx
  __int64 v13; // rax
  __int64 v14; // rax
  __int64 v15; // rdx
  __int64 v16; // rdx
  __int64 v17; // rdx
  __int64 v18; // rdi
  __int64 v19; // rcx
  __int64 v20; // rbx
  _QWORD *v21; // rcx
  __int64 v22; // xmm1_8
  __int64 v23; // rbx
  __int64 v24; // rdx
  signed int v25; // ecx
  int v26; // eax
  int v27; // eax
  int v28; // ebx
  __int128 v30; // [rsp+30h] [rbp-30h] BYREF
  __int128 v31; // [rsp+40h] [rbp-20h] BYREF
  LARGE_INTEGER PerformanceCount; // [rsp+80h] [rbp+20h] BYREF

  if ( (*(int (__fastcall **)(__int64))(*(_QWORD *)qword_1811CEAF8 + 328LL))(a1: qword_1811CEAF8) >= 0 ) /*0x1801c31fa*/
  {
    v2 = sub_1802C5260(); /*0x1801c3205*/
    PerformanceCount.QuadPart = 0; /*0x1801c320e*/
    v3 = v2; /*0x1801c3212*/
    QueryPerformanceCounter(lpPerformanceCount: &PerformanceCount); /*0x1801c3215*/
    unknown_libname_4(a1: &v31); /*0x1801c321f*/
    v4 = *(_OWORD **)(v3 + 24); /*0x1801c3236*/
    *(LARGE_INTEGER *)&v30 = PerformanceCount; /*0x1801c323a*/
    *((_QWORD *)&v30 + 1) = "Render(Scene)"; /*0x1801c3242*/
    v31 = xmmword_1812D3E10; /*0x1801c3246*/
    if ( *(_OWORD **)(v3 + 32) == v4 ) /*0x1801c324e*/
    {
      sub_1800C7E20(a1: v3 + 16, a2: v4, a3: &v30); /*0x1801c326a*/
    }
    else
    {
      *v4 = v30; /*0x1801c3254*/
      v4[1] = v31; /*0x1801c325b*/
      *(_QWORD *)(v3 + 24) += 32LL; /*0x1801c325f*/
    }
    if ( *(_QWORD *)(a1 + 176) != 0 ) /*0x1801c3279*/
      sub_180356F70(); /*0x1801c327b*/
    v5 = sub_1802C5260(); /*0x1801c3280*/
    PerformanceCount.QuadPart = 0; /*0x1801c3289*/
    v6 = v5; /*0x1801c328d*/
    QueryPerformanceCounter(lpPerformanceCount: &PerformanceCount); /*0x1801c3290*/
    unknown_libname_4(a1: &v31); /*0x1801c329a*/
    v7 = *(_OWORD **)(v6 + 24); /*0x1801c32b1*/
    *(LARGE_INTEGER *)&v30 = PerformanceCount; /*0x1801c32b5*/
    *((_QWORD *)&v30 + 1) = "Render(NBTRenderer)"; /*0x1801c32bd*/
    v31 = xmmword_1812D3E30; /*0x1801c32c1*/
    if ( *(_OWORD **)(v6 + 32) == v7 ) /*0x1801c32c9*/
    {
      sub_1800C7E20(a1: v6 + 16, a2: v7, a3: &v30); /*0x1801c32e5*/
    }
    else
    {
      *v7 = v30; /*0x1801c32cf*/
      v7[1] = v31; /*0x1801c32d6*/
      *(_QWORD *)(v6 + 24) += 32LL; /*0x1801c32da*/
    }
    sub_1803383C0(); /*0x1801c32ea*/
    v8 = sub_1800DA910(); /*0x1801c32ef*/
    sub_1800DB2A0(a1: v8); /*0x1801c32f7*/
    v9 = &unk_184AEF660; /*0x1801c32fc*/
    do /*0x1801c3367*/
    {
      if ( (dword_184AEF658 & 1) == 0 ) /*0x1801c3317*/
      {
        dword_184AEF658 |= 1u; /*0x1801c3319*/
        sub_180102B80(a1: &unk_184AEF660, a2: 0, a3: 88); /*0x1801c332d*/
        qword_184AEF6B8 = 0; /*0x1801c3339*/
        atexit(a1: sub_1803A4DD0); /*0x1801c3340*/
      }
      v10 = *v9; /*0x1801c3345*/
      if ( *(_BYTE *)(*v9 + 92LL) != 0 ) /*0x1801c334c*/
      {
        sub_180338430(a1: *v9); /*0x1801c3351*/
        (*(void (__fastcall **)(_QWORD))(**(_QWORD **)(v10 + 8) + 32LL))(a1: *(_QWORD *)(v10 + 8)); /*0x1801c335d*/
      }
      ++v9; /*0x1801c3360*/
    }
    while ( (__int64)v9 <= (__int64)&qword_184AEF688 ); /*0x1801c3367*/
    if ( *(_DWORD *)(sub_180369760() + 4) == 4 ) /*0x1801c3377*/
    {
      sub_1802BB260(a1: &off_1807075A0, a2: 5); /*0x1801c3387*/
      sub_1802BAC20(a1: &off_1807075A0); /*0x1801c339b*/
    }
    if ( *(_DWORD *)(sub_180369760() + 4) == 5 ) /*0x1801c33a9*/
    {
      sub_1802BB260(a1: &off_1807075A0, a2: 6); /*0x1801c33b9*/
      sub_1802BAC20(a1: &off_1807075A0); /*0x1801c33cd*/
    }
    if ( (dword_184ACF5E8 & 1) == 0 ) /*0x1801c33da*/
    {
      qword_184ACF5F0 = (__int64)&CPerformanceCounter::`vftable'; /*0x1801c33df*/
      xmmword_184ACF600 = 0; /*0x1801c33e9*/
      dword_184ACF5E8 |= 1u; /*0x1801c33f4*/
      xmmword_184ACF610 = 0; /*0x1801c3401*/
      xmmword_184ACF620 = 0; /*0x1801c3409*/
      word_184ACF5F8 = 0; /*0x1801c3411*/
      dword_184ACF5FC = 0; /*0x1801c3419*/
      atexit(a1: sub_1803A16D0); /*0x1801c3420*/
    }
    PerformanceCount.QuadPart = 0; /*0x1801c3429*/
    QueryPerformanceCounter(lpPerformanceCount: &PerformanceCount); /*0x1801c342d*/
    unknown_libname_4(a1: &v31); /*0x1801c3437*/
    v11 = *((_QWORD *)&xmmword_184ACF600 + 1); /*0x1801c3447*/
    *(LARGE_INTEGER *)&v30 = PerformanceCount; /*0x1801c3455*/
    *((_QWORD *)&v30 + 1) = "Render(Debug)"; /*0x1801c3460*/
    v31 = xmmword_1812D3E50; /*0x1801c3464*/
    if ( (_QWORD)xmmword_184ACF610 == *((_QWORD *)&xmmword_184ACF600 + 1) ) /*0x1801c3468*/
    {
      sub_1800C7E20(a1: &xmmword_184ACF600, a2: *((_QWORD *)&xmmword_184ACF600 + 1), a3: &v30); /*0x1801c348e*/
    }
    else
    {
      **((_OWORD **)&xmmword_184ACF600 + 1) = v30; /*0x1801c346e*/
      *(_OWORD *)(v11 + 16) = v31; /*0x1801c3475*/
      *((_QWORD *)&xmmword_184ACF600 + 1) += 32LL; /*0x1801c3479*/
    }
    if ( (dword_184ACF5E8 & 1) == 0 ) /*0x1801c349b*/
    {
      qword_184ACF5F0 = (__int64)&CPerformanceCounter::`vftable'; /*0x1801c34a4*/
      xmmword_184ACF600 = 0; /*0x1801c34ae*/
      dword_184ACF5E8 |= 1u; /*0x1801c34b9*/
      xmmword_184ACF610 = 0; /*0x1801c34c6*/
      xmmword_184ACF620 = 0; /*0x1801c34ce*/
      word_184ACF5F8 = 0; /*0x1801c34d6*/
      dword_184ACF5FC = 0; /*0x1801c34de*/
      atexit(a1: sub_1803A16D0); /*0x1801c34e5*/
      if ( (dword_184ACF5E8 & 1) == 0 ) /*0x1801c34f2*/
      {
        qword_184ACF5F0 = (__int64)&CPerformanceCounter::`vftable'; /*0x1801c34f7*/
        xmmword_184ACF600 = 0; /*0x1801c3501*/
        dword_184ACF5E8 |= 1u; /*0x1801c350c*/
        xmmword_184ACF610 = 0; /*0x1801c3519*/
        xmmword_184ACF620 = 0; /*0x1801c3521*/
        word_184ACF5F8 = 0; /*0x1801c3529*/
        dword_184ACF5FC = 0; /*0x1801c3531*/
        atexit(a1: sub_1803A16D0); /*0x1801c3538*/
      }
    }
    PerformanceCount.QuadPart = 0; /*0x1801c3541*/
    QueryPerformanceCounter(lpPerformanceCount: &PerformanceCount); /*0x1801c3545*/
    unknown_libname_4(a1: &v31); /*0x1801c354f*/
    v12 = *((_QWORD *)&xmmword_184ACF600 + 1); /*0x1801c355f*/
    *(LARGE_INTEGER *)&v30 = PerformanceCount; /*0x1801c356d*/
    *((_QWORD *)&v30 + 1) = "Render(NBTRenderer::End)"; /*0x1801c3578*/
    v31 = xmmword_1812D3E60; /*0x1801c357c*/
    if ( (_QWORD)xmmword_184ACF610 == *((_QWORD *)&xmmword_184ACF600 + 1) ) /*0x1801c3580*/
    {
      sub_1800C7E20(a1: &xmmword_184ACF600, a2: *((_QWORD *)&xmmword_184ACF600 + 1), a3: &v30); /*0x1801c35a6*/
    }
    else
    {
      **((_OWORD **)&xmmword_184ACF600 + 1) = v30; /*0x1801c3586*/
      *(_OWORD *)(v12 + 16) = v31; /*0x1801c358d*/
      *((_QWORD *)&xmmword_184ACF600 + 1) += 32LL; /*0x1801c3591*/
    }
    if ( (dword_184AEF658 & 1) == 0 ) /*0x1801c35b3*/
    {
      dword_184AEF658 |= 1u; /*0x1801c35c1*/
      sub_180102B80(a1: &unk_184AEF660, a2: 0, a3: 88); /*0x1801c35cb*/
      qword_184AEF6B8 = 0; /*0x1801c35d7*/
      atexit(a1: sub_1803A4DD0); /*0x1801c35de*/
    }
    v13 = sub_1800DA910(); /*0x1801c35e3*/
    sub_1800DB2A0(a1: v13); /*0x1801c35eb*/
    sub_1800D6490(a1: 0); /*0x1801c35f2*/
    v14 = sub_1800DA910(); /*0x1801c35f7*/
    sub_1800DB2B0(a1: v14); /*0x1801c35ff*/
  }
  if ( (dword_184ACF5E8 & 1) == 0 ) /*0x1801c360c*/
  {
    qword_184ACF5F0 = (__int64)&CPerformanceCounter::`vftable'; /*0x1801c3611*/
    xmmword_184ACF600 = 0; /*0x1801c361b*/
    dword_184ACF5E8 |= 1u; /*0x1801c3626*/
    xmmword_184ACF610 = 0; /*0x1801c3633*/
    xmmword_184ACF620 = 0; /*0x1801c363b*/
    word_184ACF5F8 = 0; /*0x1801c3643*/
    dword_184ACF5FC = 0; /*0x1801c364b*/
    atexit(a1: sub_1803A16D0); /*0x1801c3652*/
  }
  PerformanceCount.QuadPart = 0; /*0x1801c365b*/
  QueryPerformanceCounter(lpPerformanceCount: &PerformanceCount); /*0x1801c365f*/
  unknown_libname_4(a1: &v31); /*0x1801c3669*/
  v15 = *((_QWORD *)&xmmword_184ACF600 + 1); /*0x1801c3679*/
  *(LARGE_INTEGER *)&v30 = PerformanceCount; /*0x1801c3687*/
  *((_QWORD *)&v30 + 1) = "IDirect3DDevice9::EndScene()"; /*0x1801c3692*/
  v31 = xmmword_1812D3E60; /*0x1801c3696*/
  if ( (_QWORD)xmmword_184ACF610 == *((_QWORD *)&xmmword_184ACF600 + 1) ) /*0x1801c369a*/
  {
    sub_1800C7E20(a1: &xmmword_184ACF600, a2: *((_QWORD *)&xmmword_184ACF600 + 1), a3: &v30); /*0x1801c36c0*/
  }
  else
  {
    **((_OWORD **)&xmmword_184ACF600 + 1) = v30; /*0x1801c36a0*/
    *(_OWORD *)(v15 + 16) = v31; /*0x1801c36a7*/
    *((_QWORD *)&xmmword_184ACF600 + 1) += 32LL; /*0x1801c36ab*/
  }
  (*(void (__fastcall **)(__int64))(*(_QWORD *)qword_1811CEAF8 + 336LL))(a1: qword_1811CEAF8); /*0x1801c36cf*/
  if ( (dword_184ACF5E8 & 1) == 0 ) /*0x1801c36dd*/
  {
    qword_184ACF5F0 = (__int64)&CPerformanceCounter::`vftable'; /*0x1801c36e2*/
    xmmword_184ACF600 = 0; /*0x1801c36ec*/
    dword_184ACF5E8 |= 1u; /*0x1801c36f7*/
    xmmword_184ACF610 = 0; /*0x1801c3704*/
    xmmword_184ACF620 = 0; /*0x1801c370c*/
    word_184ACF5F8 = 0; /*0x1801c3714*/
    dword_184ACF5FC = 0; /*0x1801c371c*/
    atexit(a1: sub_1803A16D0); /*0x1801c3723*/
  }
  PerformanceCount.QuadPart = 0; /*0x1801c372c*/
  QueryPerformanceCounter(lpPerformanceCount: &PerformanceCount); /*0x1801c3730*/
  unknown_libname_4(a1: &v31); /*0x1801c373a*/
  v16 = *((_QWORD *)&xmmword_184ACF600 + 1); /*0x1801c374a*/
  *(LARGE_INTEGER *)&v30 = PerformanceCount; /*0x1801c3758*/
  *((_QWORD *)&v30 + 1) = "Switch"; /*0x1801c3763*/
  v31 = xmmword_1812D3E10; /*0x1801c3767*/
  if ( (_QWORD)xmmword_184ACF610 == *((_QWORD *)&xmmword_184ACF600 + 1) ) /*0x1801c376b*/
  {
    sub_1800C7E20(a1: &xmmword_184ACF600, a2: *((_QWORD *)&xmmword_184ACF600 + 1), a3: &v30); /*0x1801c3791*/
  }
  else
  {
    **((_OWORD **)&xmmword_184ACF600 + 1) = v30; /*0x1801c3771*/
    *(_OWORD *)(v16 + 16) = v31; /*0x1801c3778*/
    *((_QWORD *)&xmmword_184ACF600 + 1) += 32LL; /*0x1801c377c*/
  }
  avs2_core_8(a1: 0); /*0x1801c3798*/
  if ( (dword_184ACF5E8 & 1) == 0 ) /*0x1801c37a6*/
  {
    qword_184ACF5F0 = (__int64)&CPerformanceCounter::`vftable'; /*0x1801c37ab*/
    xmmword_184ACF600 = 0; /*0x1801c37b5*/
    dword_184ACF5E8 |= 1u; /*0x1801c37c0*/
    xmmword_184ACF610 = 0; /*0x1801c37cd*/
    xmmword_184ACF620 = 0; /*0x1801c37d5*/
    word_184ACF5F8 = 0; /*0x1801c37dd*/
    dword_184ACF5FC = 0; /*0x1801c37e5*/
    atexit(a1: sub_1803A16D0); /*0x1801c37ec*/
  }
  PerformanceCount.QuadPart = 0; /*0x1801c37f5*/
  QueryPerformanceCounter(lpPerformanceCount: &PerformanceCount); /*0x1801c37f9*/
  unknown_libname_4(a1: &v31); /*0x1801c3803*/
  v17 = *((_QWORD *)&xmmword_184ACF600 + 1); /*0x1801c3813*/
  *(LARGE_INTEGER *)&v30 = PerformanceCount; /*0x1801c3821*/
  *((_QWORD *)&v30 + 1) = "Dispatch"; /*0x1801c382c*/
  v31 = xmmword_1812D3E10; /*0x1801c3830*/
  if ( (_QWORD)xmmword_184ACF610 == *((_QWORD *)&xmmword_184ACF600 + 1) ) /*0x1801c3834*/
  {
    sub_1800C7E20(a1: &xmmword_184ACF600, a2: *((_QWORD *)&xmmword_184ACF600 + 1), a3: &v30); /*0x1801c385a*/
  }
  else
  {
    **((_OWORD **)&xmmword_184ACF600 + 1) = v30; /*0x1801c383a*/
    *(_OWORD *)(v17 + 16) = v31; /*0x1801c3841*/
    *((_QWORD *)&xmmword_184ACF600 + 1) += 32LL; /*0x1801c3845*/
  }
  v18 = *(_QWORD *)(a1 + 176); /*0x1801c385f*/
  if ( v18 != 0 ) /*0x1801c3871*/
  {
    v19 = *(_QWORD *)(v18 + 16); /*0x1801c3877*/
    *(_BYTE *)(v19 + 84) = 0; /*0x1801c387b*/
    *(_DWORD *)(v19 + 80) = 0; /*0x1801c387f*/
    if ( (unsigned __int8)((__int64 (*)(void))sub_180355CD0)() != 0 ) /*0x1801c388a*/
    {
      do /*0x1801c3907*/
      {
        v20 = *(_QWORD *)(v18 + 16); /*0x1801c3890*/
        DWORD1(v30) = 0; /*0x1801c3897*/
        if ( (unsigned __int8)sub_180355CD0(a1: v20) != 0 ) /*0x1801c38a2*/
        {
          v21 = *(_QWORD **)(v20 + 8LL * *(unsigned __int8 *)(v20 + 84) + 8); /*0x1801c38a8*/
          v22 = *(_QWORD *)(*v21 + 24LL * *(unsigned int *)(v20 + 80) + 16); /*0x1801c38bb*/
          v30 = *(_OWORD *)(*v21 + 24LL * *(unsigned int *)(v20 + 80)); /*0x1801c38c1*/
          *(_QWORD *)&v31 = v22; /*0x1801c38c5*/
        }
        if ( DWORD1(v30) == 1 || DWORD1(v30) == 4 ) /*0x1801c38da*/
          sub_180356010(a1: *(_QWORD *)(v18 + 16), a2: 2); /*0x1801c38e5*/
        v23 = *(_QWORD *)(v18 + 16); /*0x1801c38ea*/
        if ( (unsigned __int8)sub_180355CD0(a1: v23) == 0 ) /*0x1801c38f8*/
          break; /*0x1801c38f8*/
        ++*(_DWORD *)(v23 + 80); /*0x1801c38fa*/
      }
      while ( (unsigned __int8)sub_180355CD0(a1: v23) != 0 ); /*0x1801c3907*/
    }
    sub_180355170(a1: v18); /*0x1801c390c*/
  }
  if ( (dword_184ACF5E8 & 1) == 0 ) /*0x1801c391e*/
  {
    qword_184ACF5F0 = (__int64)&CPerformanceCounter::`vftable'; /*0x1801c3923*/
    xmmword_184ACF600 = 0; /*0x1801c392d*/
    dword_184ACF5E8 |= 1u; /*0x1801c3938*/
    xmmword_184ACF610 = 0; /*0x1801c3945*/
    xmmword_184ACF620 = 0; /*0x1801c394d*/
    word_184ACF5F8 = 0; /*0x1801c3955*/
    dword_184ACF5FC = 0; /*0x1801c395d*/
    atexit(a1: sub_1803A16D0); /*0x1801c3964*/
  }
  PerformanceCount.QuadPart = 0; /*0x1801c396d*/
  QueryPerformanceCounter(lpPerformanceCount: &PerformanceCount); /*0x1801c3971*/
  unknown_libname_4(a1: &v31); /*0x1801c397b*/
  v24 = *((_QWORD *)&xmmword_184ACF600 + 1); /*0x1801c398b*/
  *(LARGE_INTEGER *)&v30 = PerformanceCount; /*0x1801c3999*/
  *((_QWORD *)&v30 + 1) = "VSYNC / Present"; /*0x1801c39a4*/
  v31 = xmmword_1812D3DD0; /*0x1801c39a8*/
  if ( (_QWORD)xmmword_184ACF610 == *((_QWORD *)&xmmword_184ACF600 + 1) ) /*0x1801c39ac*/
  {
    sub_1800C7E20(a1: &xmmword_184ACF600, a2: *((_QWORD *)&xmmword_184ACF600 + 1), a3: &v30); /*0x1801c39d2*/
  }
  else
  {
    **((_OWORD **)&xmmword_184ACF600 + 1) = v30; /*0x1801c39b2*/
    *(_OWORD *)(v24 + 16) = v31; /*0x1801c39b9*/
    *((_QWORD *)&xmmword_184ACF600 + 1) += 32LL; /*0x1801c39bd*/
  }
  v25 = dword_1813E94E0 - timeGetTime(); /*0x1801c39e3*/
  if ( v25 > 12 ) /*0x1801c39ec*/
    v25 = 12; /*0x1801c39ec*/
  v26 = 0; /*0x1801c39ef*/
  if ( v25 > 0 ) /*0x1801c39f4*/
    v26 = v25; /*0x1801c39f4*/
  if ( v26 > 2 ) /*0x1801c39fa*/
    SleepEx(dwMilliseconds: v26 - 2, bAlertable: false); /*0x1801c3a01*/
  v27 = (*(__int64 (__fastcall **)(__int64, _QWORD, _QWORD, _QWORD, _QWORD))(*(_QWORD *)qword_1811CEAF8 + 136LL))( /*0x1801c3a1e*/
          a1: qword_1811CEAF8,
          a2: 0,
          a3: 0,
          a4: 0,
          a5: 0);
  v28 = v27; /*0x1801c3a24*/
  if ( v27 != 0 && v27 != dword_1813EC144 )
  {
    if ( v27 == -2005530520 )
      avs2_core_380(a1: "App", a2: "Direct3DDevice9::Present() failed: D3DERR_DEVICELOST detected");
    else
      avs2_core_380(a1: "App", a2: "Direct3DDevice9::Present() failed: D3DERR(%d) detected", v27);
    dword_1813EC144 = v28; /*0x1801c3a66*/
  }
  if ( byte_1806FC344 != 0 && dword_1811CEAF4 != 0 ) /*0x1801c3a84*/
    (*(void (__fastcall **)(_QWORD, _QWORD, _QWORD, _QWORD, _QWORD, _DWORD))(**((_QWORD **)qword_1811CED00 + 1) + 24LL))( /*0x1801c3aa6*/
      a1: *((_QWORD *)qword_1811CED00 + 1),
      a2: 0,
      a3: 0,
      a4: 0,
      a5: 0,
      a6: 0);
  if ( (dword_184ACF5E8 & 1) == 0 ) /*0x1801c3ab1*/
  {
    qword_184ACF5F0 = (__int64)&CPerformanceCounter::`vftable'; /*0x1801c3ab6*/
    xmmword_184ACF600 = 0; /*0x1801c3ac0*/
    dword_184ACF5E8 |= 1u; /*0x1801c3acb*/
    xmmword_184ACF610 = 0; /*0x1801c3ad8*/
    xmmword_184ACF620 = 0; /*0x1801c3ae0*/
    word_184ACF5F8 = 0; /*0x1801c3ae8*/
    dword_184ACF5FC = 0; /*0x1801c3af0*/
    atexit(a1: sub_1803A16D0); /*0x1801c3af7*/
  }
  return sub_1802C54E0(a1: &qword_184ACF5F0); /*0x1801c3b03*/
}
