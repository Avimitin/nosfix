char __fastcall sub_1801A43E0(__int64 a1, __int64 a2)
{
  float v4; // xmm7_4
  int v5; // ebx
  unsigned __int64 **v6; // rdx
  int v7; // ecx
  unsigned int *v8; // rsi
  unsigned int v9; // ebx
  unsigned int *v10; // r12
  int v11; // r9d
  __int64 v12; // r8
  _BYTE *v13; // rcx
  char *v14; // r14
  unsigned __int64 v15; // r12
  __int64 v16; // rax
  bool v17; // cf
  size_t v18; // rax
  unsigned __int64 *v19; // rax
  _QWORD *v20; // rbx
  __int64 v21; // r11
  int v22; // ebx
  int v23; // eax
  __int64 v24; // r8
  __int64 v25; // rax
  int *v26; // rcx
  int *v27; // rdx
  char *v28; // r8
  _OWORD *v29; // rdx
  int v30; // r13d
  __int64 v31; // r15
  __int64 v32; // r9
  __int64 v33; // r8
  int *v34; // r14
  int v35; // r8d
  int v36; // edx
  int v37; // r9d
  __int64 v38; // rbx
  __int64 v39; // rcx
  int v40; // eax
  int v41; // eax
  int v42; // eax
  unsigned __int64 v43; // rsi
  size_t v44; // rax
  _QWORD *v45; // rax
  _QWORD *v46; // rdx
  _QWORD *i; // rcx
  int v48; // r9d
  __int64 v49; // rdx
  unsigned __int64 v50; // r8
  __int64 v51; // r10
  int v52; // ecx
  int v53; // eax
  __int64 v54; // rdx
  int v55; // ecx
  __int64 v56; // rax
  __int64 v57; // rcx
  int v58; // r8d
  __int64 v59; // rdx
  int v60; // ecx
  int v61; // ecx
  bool v62; // zf
  __int64 v63; // rax
  __int64 v64; // rax
  int v65; // edx
  int v66; // ecx
  char *v67; // r8
  _DWORD *v68; // r9
  _DWORD *v69; // rcx
  __int64 v70; // rdx
  int v71; // eax
  int v72; // eax
  char *v73; // rax
  void *v74; // rcx
  __int64 v75; // rbx
  int v76; // esi
  __int64 v77; // r9
  __int64 v78; // r8
  __int16 *v79; // r10
  __int16 *v80; // r9
  __int64 v81; // r8
  int v82; // edx
  __int64 v83; // rax
  int v84; // r9d
  _DWORD *v85; // rcx
  int v86; // ecx
  __int64 v87; // rdx
  __int64 v88; // r9
  _DWORD *v89; // rax
  unsigned int v90; // ebx
  __int64 v91; // rax
  int v92; // r8d
  int v93; // ecx
  __int16 v94; // ax
  unsigned __int64 **v95; // rdx
  __int64 v96; // rcx
  __int64 v97; // rax
  __int64 v98; // rdx
  int v99; // eax
  int v100; // r8d
  int v101; // r8d
  __int64 v102; // rax
  __int64 v103; // rdx
  _QWORD *v104; // rdx
  int v105; // ebx
  __int64 v106; // r8
  int *v107; // rcx
  int v108; // eax
  int *v109; // rdx
  char *v110; // rax
  unsigned __int64 *v112; // [rsp+30h] [rbp-E8h] BYREF
  unsigned __int64 v113; // [rsp+38h] [rbp-E0h] BYREF
  void *Block[2]; // [rsp+40h] [rbp-D8h] BYREF
  __int64 v115; // [rsp+50h] [rbp-C8h]
  void *v116[2]; // [rsp+58h] [rbp-C0h] BYREF
  unsigned int *v117; // [rsp+68h] [rbp-B0h]
  unsigned int *v118; // [rsp+70h] [rbp-A8h]
  __int64 v119; // [rsp+78h] [rbp-A0h]
  _QWORD v120[8]; // [rsp+80h] [rbp-98h]

  v119 = -2; /*0x1801a43f3*/
  if ( (dword_181B7D57C & 1) == 0 ) /*0x1801a442c*/
  {
    __wind /*0xf1c00000000000f0*/
    {
      dword_181B7D57C |= 1u; /*0x1801a4431*/
      sub_1801A3930(); /*0x1801a4437*/
      atexit(a1: sub_1803928D0); /*0x1801a4443*/
    }
    __unwind
    {
      dword_181B7D57C &= ~1u; /*0x180381ad4*/
    }
  }
  if ( (unsigned __int8)sub_1801A34C0(a1: word_181B7D580, a2) == 0 ) /*0x1801a445d*/
    return 0; /*0x1801a445d*/
  v4 = *(float *)&qword_1813E471C; /*0x1801a4463*/
  v5 = (int)(float)((float)dword_181B7D58C / *(float *)&qword_1813E471C); /*0x1801a4486*/
  *(float *)&v112 = *(float *)&qword_181B7D584 * *(float *)&qword_1813E471C; /*0x1801a448a*/
  HIDWORD(v112) = 0; /*0x1801a4493*/
  v6 = *(unsigned __int64 ***)(a1 + 24); /*0x1801a449c*/
  if ( *(unsigned __int64 ***)(a1 + 32) == v6 ) /*0x1801a44a4*/
  {
    sub_180095600(a1: a1 + 16, a2: v6, a3: &v112); /*0x1801a44ba*/
  }
  else
  {
    *v6 = v112; /*0x1801a44ab*/
    *(_QWORD *)(a1 + 24) += 8LL; /*0x1801a44ae*/
  }
  *(_DWORD *)(a1 + 168) = (int)((double)v5 * *(double *)&qword_18070B2F8 / 1000.0); /*0x1801a44df*/
  *(_DWORD *)(a1 + 172) = (int)(*(double *)&qword_18070B2F8 * 0.0 / 1000.0); /*0x1801a44fc*/
  *(_DWORD *)(a1 + 144) = v5; /*0x1801a4502*/
  *(_DWORD *)(a1 + 148) = 0; /*0x1801a4508*/
  *(_DWORD *)(a1 + 152) = HIDWORD(qword_181B7D584); /*0x1801a4515*/
  *(_DWORD *)(a1 + 156) = DWORD2(xmmword_181B7D590); /*0x1801a4521*/
  *(_DWORD *)(a1 + 88) = HIDWORD(xmmword_181B7D590); /*0x1801a452d*/
  v7 = DWORD1(xmmword_181B7D590); /*0x1801a4530*/
  *(_QWORD *)(a1 + 160) = xmmword_181B7D590; /*0x1801a4543*/
  dword_1813E4714 = v7; /*0x1801a4549*/
  dword_1813E4718 = *(_DWORD *)(a1 + 160); /*0x1801a4555*/
  *(_OWORD *)v116 = 0; /*0x1801a455b*/
  v8 = nullptr; /*0x1801a4561*/
  v118 = nullptr; /*0x1801a4564*/
  v117 = nullptr; /*0x1801a4569*/
  __eh34_enter_wind_state(-1, 1); /*0xf1c00000000000dc*/
  v9 = 0; /*0x1801a456e*/
  LODWORD(v112) = 0; /*0x1801a4571*/
  v10 = nullptr; /*0x1801a4575*/
  if ( *(int *)(a1 + 152) > 0 ) /*0x1801a4581*/
  {
    v11 = 0; /*0x1801a4587*/
    do /*0x1801a4671*/
    {
      v12 = 20000; /*0x1801a45a0*/
      if ( SHIDWORD(qword_181B7D584) < 20000 ) /*0x1801a45b0*/
        v12 = HIDWORD(qword_181B7D584); /*0x1801a45b0*/
      if ( v11 < 0 || v11 >= (int)v12 ) /*0x1801a45bc*/
      {
        avs2_core_380(a1: "MusicScoreManager", a2: byte_18041A2F0, v12); /*0x1801a45db*/
        if ( (dword_184ACC190 & 1) == 0 ) /*0x1801a45e9*/
        {
          dword_184ACC190 |= 1u; /*0x1801a45ee*/
          sub_1801A2330(a1: &unk_184ACC1A0); /*0x1801a45fb*/
        }
        v13 = &unk_184ACC1D8; /*0x1801a4600*/
      }
      else
      {
        v13 = (char *)&unk_181B7D5E0 + 2464 * v11; /*0x1801a45c8*/
      }
      if ( *v13 != 0 /*0x1801a462c*/
        && *v13 <= 0x58u
        && *(int *)(sub_1801A33F0(a1: word_181B7D580, a2: v9) + 32) > -1
        && *(int *)(sub_1801A33F0(a1: word_181B7D580, a2: v9) + 32) < 3 )
      {
        if ( v8 == v10 ) /*0x1801a4631*/
        {
          sub_180078570(a1: v116, a2: v10, a3: &v112); /*0x1801a464f*/
          v8 = v117; /*0x1801a4654*/
          v10 = (unsigned int *)v116[1]; /*0x1801a4659*/
          v9 = (unsigned int)v112; /*0x1801a465e*/
        }
        else
        {
          *v10++ = v9; /*0x1801a4633*/
          v116[1] = v10; /*0x1801a463b*/
        }
      }
      LODWORD(v112) = ++v9; /*0x1801a4664*/
      v11 = v9; /*0x1801a4668*/
    }
    while ( (signed int)v9 < *(_DWORD *)(a1 + 152) ); /*0x1801a4671*/
    v118 = v8; /*0x1801a4677*/
  }
  v14 = (char *)v116[0]; /*0x1801a467c*/
  v15 = ((char *)v10 - (char *)v116[0]) >> 2; /*0x1801a4684*/
  *(_DWORD *)(a1 + 152) = v15; /*0x1801a4688*/
  __eh34_enter_try_state(1, 2); /*0xf1c00000000000d4*/
  v113 = v15; /*0x1801a468f*/
  v16 = 104 * v15; /*0x1801a4699*/
  if ( !is_mul_ok(v15, 0x68u) ) /*0x1801a4699*/
    v16 = -1; /*0x1801a46a3*/
  v17 = __CFADD__(v16, 8); /*0x1801a46a7*/
  v18 = v16 + 8; /*0x1801a46a7*/
  if ( v17 ) /*0x1801a46ab*/
    v18 = -1; /*0x1801a46ab*/
  v19 = (unsigned __int64 *)operator new(Size: v18); /*0x1801a46b2*/
  v112 = v19; /*0x1801a46b7*/
  __wind /*0xf1c00000000000c8*/
  {
    if ( v19 != nullptr ) /*0x1801a46bf*/
    {
      *v19 = v15; /*0x1801a46c1*/
      v20 = v19 + 1; /*0x1801a46c4*/
      `eh vector constructor iterator'(a1: v19 + 1, a2: 0x68u, a3: v15, a4: sub_1800A2450, a5: sub_1800A24B0); /*0x1801a46e6*/
      v21 = 0; /*0x1801a46eb*/
    }
    else
    {
      v21 = 0; /*0x1801a46f0*/
      v20 = nullptr; /*0x1801a46f3*/
    }
  }
  __unwind
  {
    is_mul_ok(v113, 0x68u); /*0x180381af6*/
    j_j_j_free(Block: v112); /*0x180381b1b*/
  }
  *(_QWORD *)(a1 + 232) = v20; /*0x1801a46f6*/
  v22 = 0; /*0x1801a46fd*/
  v23 = HIDWORD(qword_181B7D5A0); /*0x1801a4700*/
  if ( SHIDWORD(qword_181B7D5A0) <= 0 ) /*0x1801a4709*/
  {
    if ( __eh34_catch(2) ) /*0xf1c0000000000098*/
    {
catch_state_2:
      if ( __eh34_catch_ellipsis(2) ) /*0xf1c000000000009c*/
      {
        v74 = v116[0]; /*0x1801a52cb*/
        if ( v116[0] != nullptr ) /*0x1801a52d3*/
        {
          if ( (((char *)v117 - (char *)v116[0]) & 0xFFFFFFFFFFFFFFFCuLL) >= 0x1000 ) /*0x1801a52eb*/
          {
            v74 = *((void **)v116[0] - 1); /*0x1801a52f1*/
            if ( (unsigned __int64)((char *)v116[0] - (char *)v74 - 8) > 0x1F ) /*0x1801a5300*/
              invalid_parameter_noinfo_noreturn(); /*0x1801a5352*/
          }
LABEL_210:
          j_j_free(Block: v74); /*0x1801a5302*/
        }
        return 0; /*0x1801a533f*/
      }
    }
    __eh34_exit_try_state(2, 1); /*0xf1c00000000000a8*/
  }
  else
  {
    if ( __eh34_catch(2) ) /*0xf1c00000000000b0*/
      goto catch_state_2; /*0xf1c00000000000b0*/
    __eh34_exit_try_state(2, 1); /*0xf1c00000000000b4*/
    do /*0x1801a4829*/
    {
      v24 = 100; /*0x1801a4720*/
      if ( v23 < 100 ) /*0x1801a4729*/
        v24 = (unsigned int)v23; /*0x1801a4729*/
      if ( v22 < 0 || v22 >= (int)v24 ) /*0x1801a4734*/
      {
        avs2_core_380(a1: "MusicScoreManager", a2: byte_18041A430, v24, (unsigned int)v22); /*0x1801a4775*/
        if ( (dword_184ACCB78 & 1) == 0 ) /*0x1801a4783*/
        {
          dword_184ACCB78 |= 1u; /*0x1801a4788*/
          xmmword_184ACCB80 = (__int128)_mm_load_si128((const __m128i *)&xmmword_180687D30); /*0x1801a4796*/
        }
        v26 = (int *)&xmmword_184ACCB80 + 1; /*0x1801a479e*/
        v27 = (int *)&xmmword_184ACCB80 + 2; /*0x1801a47a1*/
        v28 = (char *)&xmmword_184ACCB80 + 12; /*0x1801a47a4*/
      }
      else
      {
        v25 = 16LL * v22; /*0x1801a4739*/
        v26 = (int *)((char *)&unk_184ACBB4C + v25); /*0x1801a474b*/
        v27 = (int *)((char *)&unk_184ACBB50 + v25); /*0x1801a4755*/
        v28 = (char *)&unk_184ACBB54 + v25; /*0x1801a475f*/
      }
      LODWORD(Block[0]) = v22; /*0x1801a47ab*/
      HIDWORD(Block[0]) = (int)((double)*v26 * *(double *)&qword_18070B2F8 / 1000.0); /*0x1801a47c7*/
      LODWORD(Block[1]) = (int)((double)*v27 * *(double *)&qword_18070B2F8 / 1000.0); /*0x1801a47e3*/
      HIDWORD(Block[1]) = *(_DWORD *)v28; /*0x1801a47ea*/
      v29 = *(_OWORD **)(a1 + 128); /*0x1801a47ee*/
      if ( *(_OWORD **)(a1 + 136) == v29 ) /*0x1801a47fc*/
      {
        sub_18008FAD0(a1: a1 + 120, a2: v29, a3: Block); /*0x1801a4819*/
      }
      else
      {
        *v29 = *(_OWORD *)Block; /*0x1801a4803*/
        *(_QWORD *)(a1 + 128) += 16LL; /*0x1801a4806*/
      }
      ++v22; /*0x1801a481e*/
      v23 = HIDWORD(qword_181B7D5A0); /*0x1801a4820*/
    }
    while ( v22 < SHIDWORD(qword_181B7D5A0) ); /*0x1801a4829*/
    v21 = 0; /*0x1801a482f*/
  }
  v30 = 0; /*0x1801a4832*/
  v31 = 1; /*0x1801a4835*/
  while ( v30 < (int)v15 ) /*0x1801a4845*/
  {
    v32 = *(int *)&v14[4 * v30]; /*0x1801a484e*/
    v33 = 20000; /*0x1801a4852*/
    if ( SHIDWORD(qword_181B7D584) < 20000 ) /*0x1801a4862*/
      v33 = HIDWORD(qword_181B7D584); /*0x1801a4862*/
    if ( (int)v32 < 0 || (int)v32 >= (int)v33 ) /*0x1801a486e*/
    {
      avs2_core_380(a1: "MusicScoreManager", a2: byte_18041A2F0, v33); /*0x1801a488a*/
      if ( (dword_184ACC190 & 1) == 0 ) /*0x1801a4898*/
      {
        dword_184ACC190 |= 1u; /*0x1801a489d*/
        sub_1801A2330(a1: &unk_184ACC1A0); /*0x1801a48aa*/
      }
      v34 = (int *)&unk_184ACC1A0; /*0x1801a48af*/
      LODWORD(v21) = 0; /*0x1801a48b6*/
    }
    else
    {
      v34 = (int *)((char *)&unk_181B7D5A8 + 2464 * v32); /*0x1801a4877*/
    }
    v35 = (int)(float)((float)v34[1] / v4); /*0x1801a48c6*/
    v36 = (int)(float)((float)v34[3] / v4); /*0x1801a48d8*/
    v37 = (int)(float)((float)v34[2] / v4); /*0x1801a48e9*/
    v38 = 104LL * v30; /*0x1801a48ee*/
    *(_DWORD *)(v38 + *(_QWORD *)(a1 + 232) + 56) = *v34; /*0x1801a48fc*/
    *(_DWORD *)(v38 + *(_QWORD *)(a1 + 232)) = v35; /*0x1801a4907*/
    *(_DWORD *)(v38 + *(_QWORD *)(a1 + 232) + 4) = v36; /*0x1801a4912*/
    *(_DWORD *)(v38 + *(_QWORD *)(a1 + 232) + 12) = v34[7]; /*0x1801a4921*/
    *(_DWORD *)(v38 + *(_QWORD *)(a1 + 232) + 20) = (int)((double)*(int *)(v38 + *(_QWORD *)(a1 + 232)) /*0x1801a4945*/
                                                        * *(double *)&qword_18070B2F8
                                                        / 1000.0);
    *(_DWORD *)(v38 + *(_QWORD *)(a1 + 232) + 24) = (int)((double)*(int *)(v38 + *(_QWORD *)(a1 + 232) + 4) /*0x1801a496a*/
                                                        * *(double *)&qword_18070B2F8
                                                        / 1000.0);
    *(_DWORD *)(v38 + *(_QWORD *)(a1 + 232) + 28) = (int)((double)v37 * *(double *)&qword_18070B2F8 / 1000.0); /*0x1801a498e*/
    *(_DWORD *)(v38 + *(_QWORD *)(a1 + 232) + 48) = v34[8]; /*0x1801a499d*/
    *(_DWORD *)(v38 + *(_QWORD *)(a1 + 232) + 52) = v34[9]; /*0x1801a49ac*/
    *(_DWORD *)(v38 + *(_QWORD *)(a1 + 232) + 8) = v21; /*0x1801a49b7*/
    *(_BYTE *)(v38 + *(_QWORD *)(a1 + 232) + 96) = 0; /*0x1801a49c3*/
    *(_DWORD *)(v38 + *(_QWORD *)(a1 + 232) + 88) = v34[5]; /*0x1801a49d3*/
    *(_DWORD *)(v38 + *(_QWORD *)(a1 + 232) + 92) = v34[6]; /*0x1801a49e2*/
    v39 = *(_QWORD *)(a1 + 232); /*0x1801a49e6*/
    if ( *(_DWORD *)(v39 + v38 + 52) == 1 ) /*0x1801a49f2*/
    {
      v40 = *(_DWORD *)(v39 + v38 + 88); /*0x1801a49f8*/
      if ( v40 > 28 ) /*0x1801a49fe*/
      {
        *(_DWORD *)(v39 + v38 + 88) = v40 - 28; /*0x1801a4a03*/
        v39 = *(_QWORD *)(a1 + 232); /*0x1801a4a06*/
      }
      v41 = *(_DWORD *)(v39 + v38 + 92); /*0x1801a4a0d*/
      if ( v41 > 28 ) /*0x1801a4a14*/
      {
        *(_DWORD *)(v39 + v38 + 92) = v41 - 28; /*0x1801a4a19*/
        v39 = *(_QWORD *)(a1 + 232); /*0x1801a4a1d*/
      }
    }
    if ( *(int *)(v39 + v38 + 88) < 1 ) /*0x1801a4a29*/
    {
      *(_DWORD *)(v39 + v38 + 88) = 1; /*0x1801a4a2b*/
      v39 = *(_QWORD *)(a1 + 232); /*0x1801a4a30*/
    }
    if ( *(int *)(v39 + v38 + 92) > 28 ) /*0x1801a4a3c*/
    {
      *(_DWORD *)(v39 + v38 + 92) = 28; /*0x1801a4a3e*/
      v39 = *(_QWORD *)(a1 + 232); /*0x1801a4a46*/
    }
    if ( *(int *)(v39 + v38 + 92) < 1 ) /*0x1801a4a52*/
    {
      *(_DWORD *)(v39 + v38 + 92) = 1; /*0x1801a4a54*/
      v39 = *(_QWORD *)(a1 + 232); /*0x1801a4a59*/
    }
    v42 = *(_DWORD *)(v39 + v38 + 92); /*0x1801a4a60*/
    if ( *(_DWORD *)(v39 + v38 + 88) > v42 ) /*0x1801a4a68*/
    {
      *(_DWORD *)(v39 + v38 + 88) = v42; /*0x1801a4a6a*/
      v39 = *(_QWORD *)(a1 + 232); /*0x1801a4a6e*/
    }
    if ( *(_DWORD *)(v39 + v38 + 52) == -1 ) /*0x1801a4a7a*/
    {
      *(_DWORD *)(v39 + v38 + 52) = v21; /*0x1801a4a7c*/
      v39 = *(_QWORD *)(a1 + 232); /*0x1801a4a81*/
    }
    *(_DWORD *)(v39 + v38 + 36) = v34[10]; /*0x1801a4a8c*/
    v43 = *(int *)(v38 + *(_QWORD *)(a1 + 232) + 36); /*0x1801a4a97*/
    v113 = v43; /*0x1801a4a9c*/
    v44 = 24 * v43; /*0x1801a4aa6*/
    if ( !is_mul_ok(v43, 0x18u) ) /*0x1801a4aa6*/
      v44 = -1; /*0x1801a4ab0*/
    try /*0xf1c000000000008c*/
    {
      v45 = operator new(Size: v44); /*0x1801a4ab7*/
      v46 = v45; /*0x1801a4abc*/
      v21 = 0; /*0x1801a4abf*/
      if ( v45 != nullptr ) /*0x1801a4ac5*/
      {
        for ( i = v45; v43 != 0; --v43 ) /*0x1801a4acd*/
        {
          *i = 1; /*0x1801a4ad0*/
          i[1] = 0; /*0x1801a4ad7*/
          i[2] = 0; /*0x1801a4adb*/
          i += 3; /*0x1801a4adf*/
        }
      }
      else
      {
        v46 = nullptr; /*0x1801a4aeb*/
      }
      *(_QWORD *)(v38 + *(_QWORD *)(a1 + 232) + 40) = v46; /*0x1801a4af5*/
      v48 = 0; /*0x1801a4afa*/
      v49 = *(_QWORD *)(a1 + 232); /*0x1801a4afd*/
      if ( *(int *)(v49 + v38 + 36) > 0 ) /*0x1801a4b09*/
      {
        v50 = 0; /*0x1801a4b0f*/
        do /*0x1801a4beb*/
        {
          *(_DWORD *)(*(_QWORD *)(v49 + v38 + 40) + v50 + 8) = (int)((double)v34[v50 / 4 + 12] /*0x1801a4b32*/
                                                                   * *(double *)&qword_18070B2F8
                                                                   / 1000.0);
          *(_DWORD *)(*(_QWORD *)(v38 + *(_QWORD *)(a1 + 232) + 40) + v50 + 12) = (int)((double)v34[v50 / 4 + 13] /*0x1801a4b5e*/
                                                                                      * *(double *)&qword_18070B2F8
                                                                                      / 1000.0);
          *(_DWORD *)(v50 + *(_QWORD *)(v38 + *(_QWORD *)(a1 + 232) + 40)) = LOBYTE(v34[v50 / 4 + 14]); /*0x1801a4b75*/
          *(_DWORD *)(*(_QWORD *)(v38 + *(_QWORD *)(a1 + 232) + 40) + v50 + 4) = BYTE1(v34[v50 / 4 + 14]); /*0x1801a4b8b*/
          *(_QWORD *)(*(_QWORD *)(v38 + *(_QWORD *)(a1 + 232) + 40) + v50 + 16) = *(_QWORD *)&v34[v50 / 4 + 16]; /*0x1801a4ba1*/
          v49 = *(_QWORD *)(a1 + 232); /*0x1801a4ba6*/
          v51 = *(_QWORD *)(v49 + v38 + 40); /*0x1801a4bad*/
          v52 = *(_DWORD *)(v51 + v50); /*0x1801a4bb2*/
          if ( v52 < 1 || v52 > 88 ) /*0x1801a4bbe*/
          {
            if ( v52 > 88 ) /*0x1801a4bc8*/
              v52 = 88; /*0x1801a4bc8*/
            v53 = 1; /*0x1801a4bcb*/
            if ( v52 > 1 ) /*0x1801a4bd1*/
              v53 = v52; /*0x1801a4bd1*/
            *(_DWORD *)(v51 + v50) = v53; /*0x1801a4bd4*/
            v49 = *(_QWORD *)(a1 + 232); /*0x1801a4bd8*/
          }
          ++v48; /*0x1801a4bdf*/
          v50 += 24LL; /*0x1801a4be2*/
        }
        while ( v48 < *(_DWORD *)(v49 + v38 + 36) ); /*0x1801a4beb*/
      }
      *(_DWORD *)(v49 + v38 + 16) = *((__int16 *)v34 + 8); /*0x1801a4bf6*/
      v54 = *(_QWORD *)(a1 + 232); /*0x1801a4bfa*/
      v55 = *(_DWORD *)(v38 + v54 + 12); /*0x1801a4c01*/
      if ( (v55 & 4) != 0 ) /*0x1801a4c0c*/
      {
        *(_DWORD *)(v38 + v54 + 8) = 2; /*0x1801a4c0e*/
      }
      else if ( (v55 & 0x40) != 0 ) /*0x1801a4c1f*/
      {
        *(_DWORD *)(v38 + v54 + 8) = 4; /*0x1801a4c21*/
      }
      else if ( (v55 & 2) != 0 ) /*0x1801a4c30*/
      {
        *(_DWORD *)(v38 + v54 + 8) = 1; /*0x1801a4c32*/
      }
      v56 = *(_QWORD *)(a1 + 232); /*0x1801a4c37*/
      v57 = *(int *)(v56 + v38 + 48); /*0x1801a4c3e*/
      if ( (_DWORD)v57 != -1 ) /*0x1801a4c46*/
      {
        ++*(_DWORD *)(a1 + 4 * v57 + 176); /*0x1801a4c48*/
        v56 = *(_QWORD *)(a1 + 232); /*0x1801a4c4f*/
      }
      v58 = HIDWORD(qword_1813E471C); /*0x1801a4c56*/
      *(_BYTE *)(v56 + v38 + 97) = 0; /*0x1801a4c5d*/
      v59 = *(_QWORD *)(a1 + 232); /*0x1801a4c62*/
      v60 = *(_DWORD *)(v38 + v59 + 48); /*0x1801a4c69*/
      if ( v60 != 0 ) /*0x1801a4c6f*/
      {
        v61 = v60 - 1; /*0x1801a4c71*/
        if ( v61 != 0 ) /*0x1801a4c74*/
          v62 = v61 == 1; /*0x1801a4c76*/
        else
          v62 = v58 == 2; /*0x1801a4c7b*/
      }
      else
      {
        v62 = v58 == 1; /*0x1801a4c81*/
      }
      if ( v62 ) /*0x1801a4c85*/
        *(_BYTE *)(v38 + v59 + 97) = 1; /*0x1801a4c87*/
    }
    catch ( ... )
    {
      v74 = v116[0]; /*0x1801a4dcd*/
      if ( v116[0] != nullptr ) /*0x1801a4dd5*/
      {
        if ( (((char *)v117 - (char *)v116[0]) & 0xFFFFFFFFFFFFFFFCuLL) >= 0x1000 ) /*0x1801a4df1*/
        {
          v74 = *((void **)v116[0] - 1); /*0x1801a4dfb*/
          if ( (unsigned __int64)((char *)v116[0] - (char *)v74 - 8) > 0x1F ) /*0x1801a4e0a*/
            invalid_parameter_noinfo_noreturn(); /*0x1801a5346*/
        }
        if ( __eh34_unwind(1) ) /*0xf1c0000000000064*/
          goto unwind_state_1; /*0xf1c0000000000064*/
        __eh34_exit_wind_state(1, -1); /*0xf1c0000000000068*/
        goto LABEL_210; /*0xf1c000000000006c*/
      }
      if ( __eh34_unwind(1) ) /*0xf1c0000000000050*/
        goto unwind_state_1; /*0xf1c0000000000050*/
      __eh34_exit_wind_state(1, -1); /*0xf1c000000000005c*/
      return 0; /*0xf1c0000000000060*/
    }
    v63 = *(_QWORD *)(a1 + 232); /*0x1801a4c8c*/
    if ( *(_DWORD *)(v63 + v38 + 48) > 1u ) /*0x1801a4c9a*/
      goto LABEL_108; /*0x1801a4c9a*/
    ++*(_DWORD *)(a1 + 4LL * *(int *)(v63 + v38 + 8) + 188); /*0x1801a4ca5*/
    v63 = *(_QWORD *)(a1 + 232); /*0x1801a4cac*/
    if ( *(_BYTE *)(v63 + v38 + 97) != 0 ) /*0x1801a4cb8*/
    {
      ++*(_DWORD *)(a1 + 4LL * *(int *)(v63 + v38 + 8) + 208); /*0x1801a4cbf*/
      v63 = *(_QWORD *)(a1 + 232); /*0x1801a4cc6*/
LABEL_108:
      if ( *(_BYTE *)(v63 + v38 + 97) != 0 ) /*0x1801a4cd2*/
        goto LABEL_130; /*0x1801a4cd2*/
    }
    if ( ((*(_DWORD *)(v63 + v38 + 8) - 3) & 0xFFFFFFFD) != 0 ) /*0x1801a4ce4*/
    {
      v64 = sub_18019FF80(); /*0x1801a4cea*/
      v65 = *(_DWORD *)(v64 + 200); /*0x1801a4cef*/
      if ( v65 < 0 ) /*0x1801a4cf7*/
        v65 = v21; /*0x1801a4cf7*/
      v66 = 4; /*0x1801a4cfb*/
      if ( v65 < 4 ) /*0x1801a4d02*/
        v66 = v65; /*0x1801a4d02*/
      if ( *(_DWORD *)(v64 + 40LL * v66 + 20) == 1 ) /*0x1801a4d11*/
      {
        sub_1800A25B0(a1: Block, a2: a1 + 120); /*0x1801a4d20*/
        v67 = (char *)Block[0]; /*0x1801a4d25*/
        v68 = Block[1]; /*0x1801a4d2a*/
        if ( Block[0] != Block[1] ) /*0x1801a4d32*/
        {
          v69 = (char *)Block[0] + 8; /*0x1801a4d34*/
          do /*0x1801a4d7d*/
          {
            v70 = *(_QWORD *)(a1 + 232); /*0x1801a4d40*/
            v71 = *(_DWORD *)(v70 + v38 + 20); /*0x1801a4d47*/
            if ( v71 >= *(v69 - 1) && v71 < *v69 ) /*0x1801a4d52*/
            {
              if ( *(_DWORD *)(v70 + v38 + 8) <= 1u ) /*0x1801a4d59*/
                ++*(_DWORD *)(a1 + 228); /*0x1801a4d5b*/
              v72 = 3; /*0x1801a4d61*/
              if ( v69[1] == 0 ) /*0x1801a4d6a*/
                v72 = 1; /*0x1801a4d6a*/
              *(_DWORD *)(v70 + v38 + 100) = v72; /*0x1801a4d6e*/
            }
            v69 += 4; /*0x1801a4d72*/
          }
          while ( v69 - 2 != v68 ); /*0x1801a4d7d*/
        }
        if ( v67 != nullptr ) /*0x1801a4d82*/
        {
          v73 = v67; /*0x1801a4d90*/
          if ( ((v115 - (_QWORD)v67) & 0xFFFFFFFFFFFFFFF0uLL) >= 0x1000 ) /*0x1801a4d9a*/
          {
            v67 = *((char **)v67 - 1); /*0x1801a4da0*/
            if ( (unsigned __int64)(v73 - v67 - 8) > 0x1F ) /*0x1801a4daf*/
              invalid_parameter_noinfo_noreturn(); /*0x1801a5340*/
          }
          j_j_free(Block: v67); /*0x1801a4db8*/
        }
        v21 = 0; /*0x1801a4dbd*/
      }
    }
LABEL_130:
    ++v30; /*0x1801a4dc0*/
    v14 = (char *)v116[0]; /*0x1801a4dc3*/
  }
  if ( (int)v15 > 0 ) /*0x1801a4e18*/
  {
    v75 = v21; /*0x1801a4e1e*/
    v76 = 1; /*0x1801a4e21*/
    do /*0x1801a4f70*/
    {
      v77 = *(int *)v14; /*0x1801a4e30*/
      v78 = 20000; /*0x1801a4e33*/
      if ( SHIDWORD(qword_181B7D584) < 20000 ) /*0x1801a4e43*/
        v78 = HIDWORD(qword_181B7D584); /*0x1801a4e43*/
      if ( (int)v77 < 0 || (int)v77 >= (int)v78 ) /*0x1801a4e4f*/
      {
        avs2_core_380(a1: "MusicScoreManager", a2: byte_18041A2F0, v78); /*0x1801a4e83*/
        if ( (dword_184ACC190 & 1) == 0 ) /*0x1801a4e91*/
        {
          dword_184ACC190 |= 1u; /*0x1801a4e96*/
          sub_1801A2330(a1: &unk_184ACC1A0); /*0x1801a4ea3*/
        }
        v79 = (__int16 *)&unk_184ACCB34; /*0x1801a4ea8*/
        v80 = (__int16 *)&unk_184ACCB30; /*0x1801a4eab*/
      }
      else
      {
        v79 = (__int16 *)((char *)&unk_181B7DF3C + 2464 * v77); /*0x1801a4e66*/
        v80 = (__int16 *)((char *)&unk_181B7DF38 + 2464 * v77); /*0x1801a4e70*/
      }
      v81 = *(_QWORD *)(a1 + 232); /*0x1801a4eb2*/
      if ( (*(_DWORD *)(v81 + v75 + 12) & 4) != 0 ) /*0x1801a4ec3*/
      {
        v82 = v76 - 2; /*0x1801a4ec9*/
        v83 = v31 - 2; /*0x1801a4ecc*/
        if ( v76 - 2 >= 0 ) /*0x1801a4ed2*/
        {
          v84 = *v80; /*0x1801a4ed4*/
          v85 = (_DWORD *)(v75 + v81 - 48); /*0x1801a4edc*/
          while ( *v85 != v84 ) /*0x1801a4ee3*/
          {
            --v82; /*0x1801a4ee5*/
            v85 -= 26; /*0x1801a4ee7*/
            if ( --v83 < 0 ) /*0x1801a4eef*/
              goto LABEL_153; /*0x1801a4eef*/
          }
          if ( (*(_BYTE *)(104 * v83 + v81 + 12) & 0x30) == 0 ) /*0x1801a4efd*/
            *(_QWORD *)(v81 + v75 + 64) = v81 + 104LL * v82; /*0x1801a4f09*/
        }
LABEL_153:
        v86 = v76; /*0x1801a4f0e*/
        v87 = v31; /*0x1801a4f10*/
        if ( v76 < (int)v15 ) /*0x1801a4f16*/
        {
          v88 = *(_QWORD *)(a1 + 232); /*0x1801a4f18*/
          v89 = (_DWORD *)(v75 + v88 + 160); /*0x1801a4f2a*/
          while ( *v89 != *v79 ) /*0x1801a4f33*/
          {
            ++v86; /*0x1801a4f35*/
            ++v87; /*0x1801a4f37*/
            v89 += 26; /*0x1801a4f3a*/
            if ( v86 >= (int)v15 ) /*0x1801a4f41*/
              goto LABEL_160; /*0x1801a4f41*/
          }
          if ( (*(_BYTE *)(104 * v87 + v88 + 12) & 0x30) == 0 ) /*0x1801a4f4f*/
            *(_QWORD *)(v88 + v75 + 72) = v88 + 104LL * v86; /*0x1801a4f5b*/
        }
      }
LABEL_160:
      ++v76; /*0x1801a4f60*/
      v14 += 4; /*0x1801a4f62*/
      ++v31; /*0x1801a4f66*/
      v75 += 104; /*0x1801a4f69*/
    }
    while ( v76 <= (int)v15 ); /*0x1801a4f70*/
    v14 = (char *)v116[0]; /*0x1801a4f76*/
    LODWORD(v21) = 0; /*0x1801a4f7b*/
  }
  v90 = v21; /*0x1801a4f7e*/
  if ( *(int *)(a1 + 156) > 0 ) /*0x1801a4f8f*/
  {
    while ( 2 ) /*0x1801a4f95*/
    {
      v91 = sub_1801A3350(a1: word_181B7D580, a2: v90); /*0x1801a4f95*/
      v92 = (int)(float)((float)*(int *)(v91 + 4) / v4); /*0x1801a4fae*/
      v93 = *(_DWORD *)(v91 + 8); /*0x1801a4fb3*/
      if ( v93 != 0 ) /*0x1801a4fb8*/
      {
        if ( v93 <= 0 ) /*0x1801a4fbe*/
          goto LABEL_186; /*0x1801a4fbe*/
        if ( v93 <= 8 ) /*0x1801a4fc7*/
        {
          HIDWORD(Block[0]) = (int)((double)v92 * *(double *)&qword_18070B2F8 / 1000.0); /*0x1801a50ab*/
          v101 = *(__int16 *)(v91 + 20); /*0x1801a50af*/
          LODWORD(Block[1]) = v101; /*0x1801a50b4*/
          *(__m128i *)v120 = _mm_load_si128((const __m128i *)&xmmword_180687490); /*0x1801a50c1*/
          *(__m128i *)&v120[2] = _mm_load_si128((const __m128i *)&xmmword_180687520); /*0x1801a50d2*/
          *(__m128i *)&v120[4] = _mm_load_si128((const __m128i *)&xmmword_180687570); /*0x1801a50e3*/
          *(__m128i *)&v120[6] = _mm_load_si128((const __m128i *)&xmmword_1806875C0); /*0x1801a50f4*/
          LODWORD(Block[0]) = -1; /*0x1801a5104*/
          LODWORD(v21) = 0; /*0x1801a5108*/
          v102 = 0; /*0x1801a510b*/
          while ( v93 != LODWORD(v120[v102]) ) /*0x1801a5117*/
          {
            if ( ++v102 >= 8 ) /*0x1801a5120*/
              goto LABEL_180; /*0x1801a5120*/
          }
          LODWORD(Block[0]) = HIDWORD(v120[v102]); /*0x1801a512b*/
LABEL_180:
          v103 = *(_QWORD *)(a1 + 72); /*0x1801a512f*/
          if ( *(_QWORD *)(a1 + 80) == v103 ) /*0x1801a513b*/
          {
            sub_1800A2930(a1: a1 + 64, a2: v103, a3: Block); /*0x1801a5157*/
            goto LABEL_186; /*0x1801a515c*/
          }
          *(void **)v103 = Block[0]; /*0x1801a5143*/
          *(_DWORD *)(v103 + 8) = v101; /*0x1801a5147*/
          *(_QWORD *)(a1 + 72) += 12LL; /*0x1801a514b*/
        }
        else
        {
          if ( v93 != 9 ) /*0x1801a4fd0*/
            goto LABEL_186; /*0x1801a4fd0*/
          v94 = *(_WORD *)(v91 + 20); /*0x1801a4fd6*/
          if ( v94 == 1 ) /*0x1801a4fde*/
          {
            LODWORD(v21) = 0; /*0x1801a4fe0*/
            HIDWORD(v112) = 0; /*0x1801a4fe3*/
            LODWORD(v112) = (int)((double)v92 * *(double *)&qword_18070B2F8 / 1000.0); /*0x1801a5001*/
            v95 = *(unsigned __int64 ***)(a1 + 48); /*0x1801a5009*/
            if ( *(unsigned __int64 ***)(a1 + 56) != v95 ) /*0x1801a5011*/
            {
              *v95 = v112; /*0x1801a5018*/
              *(_QWORD *)(a1 + 48) += 8LL; /*0x1801a501b*/
              goto LABEL_187; /*0x1801a5020*/
            }
            sub_18009A3C0(a1: a1 + 40, a2: v95, a3: &v112); /*0x1801a502a*/
LABEL_186:
            LODWORD(v21) = 0; /*0x1801a51b1*/
          }
          else
          {
            if ( v94 != 0 ) /*0x1801a5037*/
              goto LABEL_186; /*0x1801a5037*/
            v96 = *(_QWORD *)(a1 + 40); /*0x1801a503d*/
            v97 = (*(_QWORD *)(a1 + 48) - v96) >> 3; /*0x1801a5048*/
            if ( (int)v97 <= 0 ) /*0x1801a504e*/
              goto LABEL_186; /*0x1801a504e*/
            v98 = (int)v97; /*0x1801a5054*/
            v99 = (int)((double)v92 * *(double *)&qword_18070B2F8 / 1000.0); /*0x1801a506c*/
            *(_DWORD *)(v96 + 8 * v98 - 4) = v99; /*0x1801a5070*/
            v100 = *(_DWORD *)(v96 + 8 * v98 - 8); /*0x1801a5074*/
            LODWORD(v21) = 0; /*0x1801a5079*/
            if ( v100 >= v99 ) /*0x1801a507f*/
              *(_DWORD *)(v96 + 8 * v98 - 4) = v100 + 1; /*0x1801a5089*/
          }
        }
LABEL_187:
        if ( (signed int)++v90 >= *(_DWORD *)(a1 + 156) ) /*0x1801a51bc*/
          goto LABEL_188; /*0x1801a51bc*/
        continue; /*0x1801a51bc*/
      }
      break;
    }
    *(float *)&v113 = *(float *)(v91 + 16) * v4; /*0x1801a5167*/
    HIDWORD(v113) = (int)((double)v92 * *(double *)&qword_18070B2F8 / 1000.0); /*0x1801a5186*/
    v104 = *(_QWORD **)(a1 + 24); /*0x1801a518e*/
    if ( *(_QWORD **)(a1 + 32) == v104 ) /*0x1801a5196*/
    {
      sub_180095600(a1: a1 + 16, a2: v104, a3: &v113); /*0x1801a51ac*/
    }
    else
    {
      *v104 = v113; /*0x1801a519d*/
      *(_QWORD *)(a1 + 24) += 8LL; /*0x1801a51a0*/
    }
    goto LABEL_186; /*0x1801a51a5*/
  }
LABEL_188:
  v105 = v21; /*0x1801a51c2*/
  if ( *(int *)(a1 + 88) > 0 ) /*0x1801a51c9*/
  {
    do /*0x1801a5283*/
    {
      v106 = 10000; /*0x1801a51e0*/
      if ( SHIDWORD(xmmword_181B7D590) < 10000 ) /*0x1801a51f0*/
        v106 = HIDWORD(xmmword_181B7D590); /*0x1801a51f0*/
      if ( v105 < 0 || v105 >= (int)v106 ) /*0x1801a51fb*/
      {
        avs2_core_380(a1: "MusicScoreManager", a2: byte_18041A478, v106, (unsigned int)v105); /*0x1801a521e*/
        if ( (dword_184ACCB68 & 1) == 0 ) /*0x1801a522c*/
        {
          dword_184ACCB68 |= 1u; /*0x1801a5231*/
          qword_184ACCB70 = 0; /*0x1801a5237*/
        }
        v107 = (int *)&qword_184ACCB70 + 1; /*0x1801a523e*/
      }
      else
      {
        v107 = (int *)&word_181B7D580[4 * v105 + 24760022]; /*0x1801a5208*/
      }
      v108 = (int)((double)*v107 * *(double *)&qword_18070B2F8 / 1000.0); /*0x1801a5255*/
      LODWORD(v112) = v108; /*0x1801a5259*/
      v109 = *(int **)(a1 + 104); /*0x1801a525d*/
      if ( *(int **)(a1 + 112) == v109 ) /*0x1801a5265*/
      {
        sub_1800A2730(a1: a1 + 96, a2: v109, a3: &v112); /*0x1801a5279*/
      }
      else
      {
        *v109 = v108; /*0x1801a5267*/
        *(_QWORD *)(a1 + 104) += 4LL; /*0x1801a5269*/
      }
      ++v105; /*0x1801a527e*/
    }
    while ( v105 < *(_DWORD *)(a1 + 88) ); /*0x1801a5283*/
  }
  if ( __eh34_unwind(1) ) /*0xf1c0000000000070*/
  {
unwind_state_1:
    sub_180078230(a1: v116); /*0xf1c0000000000054*/
    __eh34_propagate_exception_into_caller(1, -1); /*0xf1c0000000000058*/
  }
  __eh34_exit_wind_state(1, -1); /*0xf1c0000000000074*/
  if ( v14 != nullptr ) /*0x1801a528c*/
  {
    v110 = v14; /*0x1801a529a*/
    if ( (((char *)v118 - v14) & 0xFFFFFFFFFFFFFFFCuLL) >= 0x1000 ) /*0x1801a52a4*/
    {
      v14 = *((char **)v14 - 1); /*0x1801a52aa*/
      if ( (unsigned __int64)(v110 - v14 - 8) > 0x1F ) /*0x1801a52b9*/
        invalid_parameter_noinfo_noreturn(); /*0x1801a534c*/
    }
    j_j_free(Block: v14); /*0x1801a52c2*/
  }
  return 1; /*0x1801a5309*/
}
