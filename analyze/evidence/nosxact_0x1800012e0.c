__int64 __fastcall sub_1800012E0(__int64 a1, __int64 a2, int a3)
{
  __int64 v6; // rdx
  __int64 v7; // r8
  void *v8; // rdx
  __int64 **v10; // r14
  int v11; // ebx
  __int64 v12; // r8
  __int64 v13; // rdx
  __int64 (__fastcall **v14)(int, int, int, int, __int64); // r15
  HRESULT Instance; // ebx
  __int64 v16; // r8
  void *v17; // rdx
  __int64 v18; // rdx
  __int64 v19; // rdx
  int FX; // esi
  __int64 v21; // r8
  __int64 v22; // rdx
  __int64 v23; // rcx
  float v24; // xmm0_4
  __int64 v25; // r8
  __int64 v26; // r8
  __int64 v27; // rdx
  int v28; // ebx
  __int64 v29; // r8
  void *v30; // rdx
  __int64 *v31; // rcx
  __int64 *v32; // rcx
  __int64 v33; // rax
  int v34; // r9d
  int v35; // xmm0_4
  int v36; // edx
  __int64 *v37; // rcx
  int v38; // r9d
  __int64 v39; // rdx
  __int64 *v40; // rcx
  _BYTE *v41; // rax
  __int64 v42; // rax
  int v43; // ebx
  __int64 v44; // r8
  int v45; // r9d
  __int64 v46; // rdx
  int *v47; // rax
  __int64 v48; // rax
  __int64 (__fastcall *v49)(int, int, int, int, __int64); // rax
  int v50; // r9d
  int v51; // r9d
  void *v52; // rdx
  __int64 *v53; // rcx
  __int64 v54; // rax
  __int64 v55; // rax
  __int64 (__fastcall *v56)(int, int, int, int, __int64); // rax
  _QWORD *v57; // r15
  _QWORD *v58; // rsi
  __int64 v59; // rax
  int *v60; // rax
  _BYTE *v61; // rax
  __int64 v62; // rax
  __int64 v63; // rax
  __int64 v64; // rax
  __int64 (__fastcall *v65)(int, int, int, int, __int64); // rax
  __int64 v66; // rax
  __int64 *v67; // rcx
  __int64 **v68; // r12
  __int64 *v69; // rcx
  __int64 v70; // rdx
  __int64 v71; // rax
  __int64 v72; // rdx
  int v73; // r13d
  __int64 v74; // rdx
  int v75; // r12d
  __int64 v76; // rdx
  int v77; // r15d
  __int64 v78; // rdx
  int v79; // esi
  _QWORD *v80; // r15
  __int64 v81; // r8
  void *v82; // rdx
  int v83; // eax
  int v84; // eax
  __int64 *v85; // rcx
  __int64 **v86; // rsi
  __int64 v87; // rdx
  int v88; // eax
  __int64 v89; // rdx
  unsigned int v90; // eax
  __int64 v91; // rdx
  LPVOID v92; // rax
  __int64 v93; // rdx
  __int64 v94; // rdx
  __int64 v95; // rcx
  __int64 *v96; // rcx
  __int64 v97; // rax
  _QWORD *v98; // rbx
  __int64 i; // rdi
  __int64 *v100; // rcx
  __int64 v101; // rbx
  unsigned __int16 v102; // ax
  int v104; // [rsp+48h] [rbp-B8h] BYREF
  int *v105; // [rsp+4Ch] [rbp-B4h]
  int v106; // [rsp+58h] [rbp-A8h] BYREF
  int *v107; // [rsp+5Ch] [rbp-A4h]
  __int64 (__fastcall *v108)(int, int, int, int, __int64); // [rsp+68h] [rbp-98h] BYREF
  __int64 v109; // [rsp+70h] [rbp-90h]
  _BYTE v110[12]; // [rsp+78h] [rbp-88h] BYREF
  int v111; // [rsp+84h] [rbp-7Ch]
  __int64 v112; // [rsp+88h] [rbp-78h] BYREF
  int v113; // [rsp+90h] [rbp-70h]
  int v114; // [rsp+94h] [rbp-6Ch]
  int v115; // [rsp+98h] [rbp-68h] BYREF
  __int64 *v116; // [rsp+9Ch] [rbp-64h]
  int v117; // [rsp+B0h] [rbp-50h] BYREF
  __int64 v118; // [rsp+B8h] [rbp-48h]
  int v119; // [rsp+C0h] [rbp-40h]
  int v120; // [rsp+C4h] [rbp-3Ch]
  __int64 (__fastcall *v121)(int, int, int, int, __int64); // [rsp+D0h] [rbp-30h]
  BOOL (__stdcall *v122)(HINSTANCE, DWORD, LPVOID); // [rsp+D8h] [rbp-28h]
  __int64 v123; // [rsp+E0h] [rbp-20h]
  __int64 *v124; // [rsp+F0h] [rbp-10h]
  __int64 v125; // [rsp+F8h] [rbp-8h]
  _DWORD v126[4]; // [rsp+100h] [rbp+0h] BYREF
  __int16 v127; // [rsp+110h] [rbp+10h]
  char v128[1030]; // [rsp+120h] [rbp+20h] BYREF
  unsigned __int16 v129; // [rsp+526h] [rbp+426h]
  int v130; // [rsp+550h] [rbp+450h] BYREF
  _BYTE *v131; // [rsp+554h] [rbp+454h]
  int v132; // [rsp+55Ch] [rbp+45Ch]
  __int64 v133; // [rsp+560h] [rbp+460h]
  int v134; // [rsp+568h] [rbp+468h]
  __int64 v135; // [rsp+56Ch] [rbp+46Ch]
  int v136; // [rsp+574h] [rbp+474h]
  __int64 v137; // [rsp+578h] [rbp+478h]
  _BYTE v138[12]; // [rsp+580h] [rbp+480h] BYREF
  int v139; // [rsp+58Ch] [rbp+48Ch]
  __int64 v140; // [rsp+590h] [rbp+490h]
  CHAR Buffer[272]; // [rsp+5A0h] [rbp+4A0h] BYREF

  avs2_core_244(a1: &v117, a2: 0, a3: 80); /*0x18000131d*/
  if ( a2 == 0 ) /*0x180001326*/
  {
    v8 = &unk_18001B570; /*0x180001328*/
LABEL_3:
    avs2_core_380(a1: "Class_XAudioControl::Init", a2: v8, a3: v7); /*0x18000132f*/
    return 2147942487LL; /*0x180001341*/
  }
  sub_1800027D0(a1, a2: v6); /*0x180001349*/
  *(_DWORD *)(a1 + 88380) = a3; /*0x180001360*/
  if ( (int)avs2_core_53(a1: &unk_180052D30, a2: 150000000, a3: 64) < 0 ) /*0x18000136e*/
  {
    v8 = &unk_18001B5B0; /*0x180001370*/
    goto LABEL_3; /*0x180001377*/
  }
  v10 = (__int64 **)(a1 + 88392); /*0x180001381*/
  v11 = sub_180003730(a1: a1 + 88392, a2: 0, a3: 0xFFFFFFFFLL); /*0x180001396*/
  if ( v11 < 0 ) /*0x18000139a*/
  {
    avs2_core_380(a1: "Class_XAudioControl::Init", a2: &unk_18001B5D0, a3: v12); /*0x1800013aa*/
    sub_1800027D0(a1, a2: v13); /*0x1800013b3*/
    return (unsigned int)v11; /*0x1800013ba*/
  }
  (*(void (__fastcall **)(__int64 *, _QWORD, char *))(**v10 + 32))(a1: *v10, a2: 0, a3: v128); /*0x1800013db*/
  v14 = (__int64 (__fastcall **)(int, int, int, int, __int64))(a1 + 88560); /*0x1800013de*/
  Instance = CoCreateInstance( /*0x180001409*/
               rclsid: &rclsid,
               pUnkOuter: nullptr,
               dwClsContext: 1u,
               riid: &riid,
               ppv: (LPVOID *)(a1 + 88560));
  if ( Instance < 0 ) /*0x18000140d*/
  {
    v17 = &unk_18001B5F0; /*0x18000140f*/
LABEL_14:
    avs2_core_380(a1: "Class_XAudioControl::Init", a2: v17, a3: v16); /*0x18000144a*/
    sub_1800027D0(a1, a2: v19); /*0x18000145a*/
    return (unsigned int)Instance; /*0x180001461*/
  }
  if ( *(_DWORD *)(a1 + 88380) != 0 ) /*0x180001426*/
  {
    Instance = CreateFX(a1: &unk_18001B498, a2: a1 + 88576); /*0x18000143d*/
    if ( Instance < 0 ) /*0x180001441*/
    {
      v17 = &unk_18001B618; /*0x180001443*/
      goto LABEL_14; /*0x180001443*/
    }
    v18 = a1 + 88584; /*0x180001466*/
  }
  else
  {
    v18 = a1 + 88568; /*0x180001428*/
  }
  FX = CreateFX(a1: &unk_18001B498, a2: v18); /*0x180001479*/
  if ( FX < 0 ) /*0x18000147d*/
  {
    avs2_core_380(a1: "Class_XAudioControl::Init", a2: &unk_18001B618, a3: v21); /*0x18000148d*/
    sub_1800027D0(a1, a2: v22); /*0x180001496*/
    return (unsigned int)FX; /*0x18000149d*/
  }
  avs2_core_244(a1: a1 + 52, a2: 0, a3: 12); /*0x1800014bc*/
  avs2_core_244(a1: a1 + 64, a2: 0, a3: 12); /*0x1800014cc*/
  avs2_core_244(a1: a1 + 76, a2: 0, a3: 12); /*0x1800014dc*/
  *(_DWORD *)(a1 + 88) = 0; /*0x1800014f2*/
  v24 = sub_1800037D0(a1: v23, a2: 48000); /*0x1800014f9*/
  *(_QWORD *)(a1 + 96) = 1065353216; /*0x18000150c*/
  *(_QWORD *)(a1 + 108) = 1065353216; /*0x180001514*/
  *(float *)(a1 + 92) = v24; /*0x18000151c*/
  *(float *)(a1 + 104) = v24; /*0x180001521*/
  *(float *)(a1 + 116) = v24; /*0x180001526*/
  *(_DWORD *)(a1 + 120) = 1065353216; /*0x18000152b*/
  CreateFX(a1: &unk_18001B4A8, a2: a1 + 88592); /*0x180001532*/
  v112 = *(_QWORD *)(a1 + 88592); /*0x18000153e*/
  v113 = 0; /*0x18000154d*/
  v115 = 1; /*0x180001551*/
  v116 = &v112; /*0x180001555*/
  v114 = v129; /*0x180001559*/
  if ( v129 == 2 ) /*0x18000155f*/
  {
    v26 = 2; /*0x18000156b*/
  }
  else
  {
    if ( v129 != 6 ) /*0x180001564*/
      goto LABEL_23; /*0x180001564*/
    v26 = 6; /*0x180001566*/
  }
  FX = (*(__int64 (__fastcall **)(__int64 *, __int64, __int64, __int64, _DWORD, _DWORD, _QWORD))(**v10 + 80))( /*0x180001596*/
         a1: *v10,
         a2: a1 + 88400,
         a3: v26,
         a4: 44100,
         a5: 0,
         a6: 0,
         a7: 0);
LABEL_23:
  if ( FX < 0 ) /*0x18000159a*/
  {
    avs2_core_380(a1: "Class_XAudioControl::Init", a2: &unk_18001B640, a3: v25); /*0x1800015aa*/
    sub_1800027D0(a1, a2: v27); /*0x1800015b3*/
    return (unsigned int)FX; /*0x1800015ba*/
  }
  v28 = (*(__int64 (__fastcall **)(_QWORD, int *))(**(_QWORD **)(a1 + 88400) + 16LL))( /*0x1800015d0*/
          a1: *(_QWORD *)(a1 + 88400),
          a2: &v115);
  if ( v28 < 0 ) /*0x1800015d4*/
  {
    v30 = &unk_18001B678; /*0x1800015d6*/
LABEL_43:
    avs2_core_380(a1: "Class_XAudioControl::Init", a2: v30, a3: v29); /*0x180001866*/
    sub_1800027D0(a1, a2: v39); /*0x180001876*/
    return (unsigned int)v28; /*0x18000187d*/
  }
  v31 = *v10; /*0x1800015e9*/
  v131 = *(_BYTE **)(a1 + 88400); /*0x1800015f1*/
  *(_DWORD *)(a1 + 88616) = 0; /*0x180001609*/
  v130 = 0; /*0x180001610*/
  v105 = &v130; /*0x180001617*/
  v104 = 1; /*0x18000161c*/
  v28 = (*(__int64 (__fastcall **)(__int64 *, __int64, __int64, __int64, _DWORD, int, int *, _QWORD))(*v31 + 72))( /*0x180001647*/
          a1: v31,
          a2: a1 + 88408,
          a3: 2,
          a4: 44100,
          a5: 0,
          a6: 5,
          a7: &v104,
          a8: 0);
  if ( v28 < 0 ) /*0x18000164b*/
  {
    v30 = &unk_18001B6A8; /*0x18000164d*/
    goto LABEL_43; /*0x180001654*/
  }
  v28 = (*(__int64 (__fastcall **)(__int64 *, __int64, __int64, __int64, _DWORD, int, int *, _QWORD))(**v10 + 72))( /*0x180001691*/
          a1: *v10,
          a2: a1 + 88416,
          a3: 2,
          a4: 44100,
          a5: 0,
          a6: 5,
          a7: &v104,
          a8: 0);
  if ( v28 < 0 ) /*0x180001695*/
  {
    v30 = &unk_18001B6E0; /*0x180001697*/
    goto LABEL_43; /*0x18000169e*/
  }
  v32 = *v10; /*0x1800016aa*/
  v131 = *(_BYTE **)(a1 + 88408); /*0x1800016b2*/
  v130 = 0; /*0x1800016ca*/
  v104 = 1; /*0x1800016d1*/
  v105 = &v130; /*0x1800016d6*/
  v33 = *v32; /*0x1800016db*/
  if ( *(_DWORD *)(a1 + 88380) == 0 ) /*0x180001701*/
  {
    v28 = (*(__int64 (__fastcall **)(__int64 *, __int64, __int64, __int64, int, int, int *, _QWORD))(v33 + 72))( /*0x18000170d*/
            a1: v32,
            a2: a1 + 88432,
            a3: 2,
            a4: 44100,
            a5: 8,
            a6: 4,
            a7: &v104,
            a8: 0);
    if ( v28 >= 0 ) /*0x180001711*/
    {
      v35 = *(_DWORD *)(a1 + 96); /*0x18000171c*/
      v36 = 0; /*0x180001721*/
      goto LABEL_36; /*0x180001723*/
    }
LABEL_42:
    v30 = &unk_18001B710; /*0x18000185f*/
    goto LABEL_43; /*0x18000185f*/
  }
  v28 = (*(__int64 (__fastcall **)(__int64 *, __int64, __int64, __int64, int, int, int *, _QWORD))(v33 + 72))( /*0x18000172f*/
          a1: v32,
          a2: a1 + 88448,
          a3: 2,
          a4: 44100,
          a5: 8,
          a6: 4,
          a7: &v104,
          a8: 0);
  if ( v28 < 0 ) /*0x180001733*/
    goto LABEL_42; /*0x180001733*/
  v35 = *(_DWORD *)(a1 + 120); /*0x18000173e*/
  v36 = 1; /*0x180001743*/
LABEL_36:
  sub_180002F40(a1, a2: v36, a3: v29, a4: v34, a5: v35); /*0x180001746*/
  v28 = (*(__int64 (__fastcall **)(__int64 *, __int64, __int64, __int64, _DWORD, int, int *, _QWORD))(**v10 + 72))( /*0x180001792*/
          a1: *v10,
          a2: a1 + 88528,
          a3: 2,
          a4: 44100,
          a5: 0,
          a6: 4,
          a7: &v104,
          a8: 0);
  if ( v28 < 0 ) /*0x180001796*/
  {
    v30 = &unk_18001B740; /*0x180001798*/
    goto LABEL_43; /*0x18000179f*/
  }
  v28 = (*(__int64 (__fastcall **)(__int64 *, __int64, __int64, __int64, _DWORD, int, int *, _QWORD))(**v10 + 72))( /*0x1800017dc*/
          a1: *v10,
          a2: a1 + 88424,
          a3: 2,
          a4: 44100,
          a5: 0,
          a6: 4,
          a7: &v104,
          a8: 0);
  if ( v28 < 0 ) /*0x1800017e0*/
  {
    v30 = &unk_18001B770; /*0x1800017e2*/
    goto LABEL_43; /*0x1800017e9*/
  }
  if ( *(_DWORD *)(a1 + 88380) != 0 ) /*0x1800017f2*/
  {
    v37 = *v10; /*0x1800017ff*/
    v131 = *(_BYTE **)(a1 + 88448); /*0x180001807*/
    v130 = 0; /*0x18000181f*/
    v104 = 1; /*0x180001826*/
    v105 = &v130; /*0x18000182b*/
    v28 = (*(__int64 (__fastcall **)(__int64 *, __int64, __int64, __int64, int, int, int *, _QWORD))(*v37 + 72))( /*0x180001859*/
            a1: v37,
            a2: a1 + 88440,
            a3: 2,
            a4: 44100,
            a5: 8,
            a6: 3,
            a7: &v104,
            a8: 0);
    if ( v28 < 0 ) /*0x18000185d*/
      goto LABEL_42; /*0x18000185d*/
    sub_180002F40(a1, a2: 0, a3: v29, a4: v38, a5: *(_DWORD *)(a1 + 108)); /*0x18000189d*/
  }
  v40 = *v10; /*0x1800018a2*/
  v104 = 1; /*0x1800018ad*/
  if ( *(_DWORD *)(a1 + 88380) == 0 ) /*0x1800018bf*/
  {
    v41 = *(_BYTE **)(a1 + 88432); /*0x1800018c5*/
    v130 = 0; /*0x1800018d1*/
    v131 = v41; /*0x1800018dd*/
    *(_DWORD *)&v110[8] = 1; /*0x1800018f5*/
    v105 = &v130; /*0x1800018f9*/
    v42 = *(_QWORD *)(a1 + 88568); /*0x1800018fe*/
    v111 = 2; /*0x180001905*/
    *(_QWORD *)v110 = v42; /*0x18000190c*/
    v106 = 1; /*0x180001916*/
    v107 = (int *)v110; /*0x18000191b*/
    v43 = (*(__int64 (__fastcall **)(__int64 *, __int64, __int64, __int64, _DWORD, int, int *, int *))(*v40 + 72))( /*0x180001940*/
            a1: v40,
            a2: a1 + 88456,
            a3: 2,
            a4: 44100,
            a5: 0,
            a6: 2,
            a7: &v104,
            a8: &v106);
    if ( v43 >= 0 ) /*0x180001944*/
    {
      sub_180002C90(a1, a2: 0, a3: v44, a4: v45, a5: 1140457472); /*0x180001968*/
      v46 = a1 + 88480; /*0x18000196d*/
      goto LABEL_53; /*0x180001974*/
    }
LABEL_51:
    v52 = &unk_18001B798; /*0x180001adb*/
LABEL_118:
    avs2_core_380(a1: "Class_XAudioControl::Init", a2: v52, a3: v44); /*0x180002574*/
LABEL_119:
    sub_1800027D0(a1, a2: v87); /*0x180002581*/
    return (unsigned int)v43; /*0x18000258b*/
  }
  v47 = *(int **)(a1 + 88440); /*0x180001979*/
  v106 = 0; /*0x180001987*/
  v107 = v47; /*0x18000198c*/
  *(_DWORD *)&v138[8] = 1; /*0x1800019a5*/
  v105 = &v106; /*0x1800019ac*/
  v48 = *(_QWORD *)(a1 + 88576); /*0x1800019b1*/
  v139 = 2; /*0x1800019b8*/
  *(_QWORD *)v138 = v48; /*0x1800019c2*/
  v130 = 1; /*0x1800019d0*/
  v131 = v138; /*0x1800019d7*/
  v49 = *(__int64 (__fastcall **)(int, int, int, int, __int64))(a1 + 88584); /*0x1800019de*/
  v109 = 0x200000001LL; /*0x1800019e5*/
  v108 = v49; /*0x1800019ea*/
  *(_DWORD *)v110 = 1; /*0x1800019fc*/
  *(_QWORD *)&v110[4] = &v108; /*0x180001a01*/
  v43 = (*(__int64 (__fastcall **)(__int64 *, __int64, __int64, __int64, _DWORD, int, int *, int *))(*v40 + 72))( /*0x180001a26*/
          a1: v40,
          a2: a1 + 88464,
          a3: 2,
          a4: 44100,
          a5: 0,
          a6: 2,
          a7: &v104,
          a8: &v130);
  if ( v43 < 0 ) /*0x180001a2a*/
    goto LABEL_51; /*0x180001a2a*/
  sub_180002C90(a1, a2: 0, a3: v44, a4: v50, a5: 1140457472); /*0x180001a51*/
  v43 = (*(__int64 (__fastcall **)(__int64 *, __int64, __int64, __int64, _DWORD, int, int *, _QWORD))(**v10 + 72))( /*0x180001a8e*/
          a1: *v10,
          a2: a1 + 88488,
          a3: 2,
          a4: 44100,
          a5: 0,
          a6: 2,
          a7: &v104,
          a8: 0);
  if ( v43 < 0 ) /*0x180001a92*/
  {
LABEL_54:
    v52 = &unk_18001B7D0; /*0x180001b3c*/
    goto LABEL_118; /*0x180001b43*/
  }
  v43 = (*(__int64 (__fastcall **)(__int64 *, __int64, __int64, __int64, _DWORD, int, int *, _BYTE *))(**v10 + 72))( /*0x180001ad5*/
          a1: *v10,
          a2: a1 + 88472,
          a3: 2,
          a4: 44100,
          a5: 0,
          a6: 2,
          a7: &v104,
          a8: v110);
  if ( v43 < 0 ) /*0x180001ad9*/
    goto LABEL_51; /*0x180001ad9*/
  sub_180002C90(a1, a2: 1, a3: v44, a4: v51, a5: 1140457472); /*0x180001af9*/
  v46 = a1 + 88496; /*0x180001afe*/
LABEL_53:
  v43 = (*(__int64 (__fastcall **)(__int64 *, __int64, __int64, __int64, _DWORD, int, int *, _QWORD))(**v10 + 72))( /*0x180001b05*/
          a1: *v10,
          a2: v46,
          a3: 2,
          a4: 44100,
          a5: 0,
          a6: 2,
          a7: &v104,
          a8: 0);
  if ( v43 < 0 ) /*0x180001b3a*/
    goto LABEL_54; /*0x180001b3a*/
  v53 = *v10; /*0x180001b48*/
  v109 = 0x200000001LL; /*0x180001b50*/
  *(_DWORD *)v110 = 1; /*0x180001b71*/
  if ( *(_DWORD *)(a1 + 88380) == 0 ) /*0x180001b8e*/
  {
    v54 = *(_QWORD *)(a1 + 88480); /*0x180001b94*/
    *(_DWORD *)v138 = 0; /*0x180001b9b*/
    v139 = 0; /*0x180001ba2*/
    *(_QWORD *)&v138[4] = v54; /*0x180001ba9*/
    v55 = *(_QWORD *)(a1 + 88456); /*0x180001bb0*/
    v104 = 2; /*0x180001bb7*/
    v140 = v55; /*0x180001bbc*/
    v105 = (int *)v138; /*0x180001bca*/
    v56 = *v14; /*0x180001bcf*/
    v57 = (_QWORD *)(a1 + 88512); /*0x180001bd2*/
    v108 = v56; /*0x180001bd9*/
    *(_QWORD *)&v110[4] = &v108; /*0x180001be6*/
    v43 = (*(__int64 (__fastcall **)(__int64 *, __int64, __int64, __int64, _DWORD, int, int *, _BYTE *))(*v53 + 72))( /*0x180001bf1*/
            a1: v53,
            a2: a1 + 88512,
            a3: 2,
            a4: 44100,
            a5: 0,
            a6: 1,
            a7: &v104,
            a8: v110);
    if ( v43 >= 0 ) /*0x180001bf5*/
    {
      v58 = (_QWORD *)(a1 + 88520); /*0x180001c10*/
      v43 = (*(__int64 (__fastcall **)(__int64 *, __int64, __int64, __int64, _DWORD, int, int *, _QWORD))(**v10 + 72))( /*0x180001c33*/
              a1: *v10,
              a2: a1 + 88520,
              a3: 2,
              a4: 44100,
              a5: 0,
              a6: 1,
              a7: &v104,
              a8: 0);
      if ( v43 >= 0 ) /*0x180001c37*/
      {
        sub_180002AC0(a1: &unk_180029254, a2: a1); /*0x180001c47*/
        (*(void (__fastcall **)(_QWORD, _QWORD, __int64, __int64, _DWORD))(*(_QWORD *)*v57 + 48LL))( /*0x180001c62*/
          a1: *v57,
          a2: 0,
          a3: a1,
          a4: 52,
          a5: 0);
        v131 = (_BYTE *)*v57; /*0x180001c68*/
        v59 = *v58; /*0x180001c6f*/
        v130 = 0; /*0x180001c72*/
        v133 = v59; /*0x180001c79*/
        v60 = &v130; /*0x180001c80*/
        v132 = 0; /*0x180001c87*/
        goto LABEL_63; /*0x180001c8e*/
      }
LABEL_61:
      v52 = &unk_18001B838; /*0x180001d65*/
      goto LABEL_118; /*0x180001d6c*/
    }
LABEL_64:
    v52 = &unk_18001B808; /*0x180001e10*/
    goto LABEL_118; /*0x180001e17*/
  }
  v61 = *(_BYTE **)(a1 + 88488); /*0x180001c93*/
  v130 = 0; /*0x180001c9a*/
  v132 = 0; /*0x180001ca1*/
  v131 = v61; /*0x180001ca8*/
  v62 = *(_QWORD *)(a1 + 88496); /*0x180001caf*/
  v134 = 0; /*0x180001cb6*/
  v133 = v62; /*0x180001cbd*/
  v63 = *(_QWORD *)(a1 + 88464); /*0x180001cc4*/
  v136 = 0; /*0x180001ccb*/
  v135 = v63; /*0x180001cd2*/
  v64 = *(_QWORD *)(a1 + 88472); /*0x180001cd9*/
  v104 = 4; /*0x180001ce0*/
  v137 = v64; /*0x180001ce8*/
  v105 = &v130; /*0x180001cf6*/
  v65 = *v14; /*0x180001cfb*/
  v57 = (_QWORD *)(a1 + 88512); /*0x180001cfe*/
  v108 = v65; /*0x180001d05*/
  *(_QWORD *)&v110[4] = &v108; /*0x180001d12*/
  v43 = (*(__int64 (__fastcall **)(__int64 *, __int64, __int64, __int64, _DWORD, int, int *, _BYTE *))(*v53 + 72))( /*0x180001d1d*/
          a1: v53,
          a2: a1 + 88512,
          a3: 2,
          a4: 44100,
          a5: 0,
          a6: 1,
          a7: &v104,
          a8: v110);
  if ( v43 < 0 ) /*0x180001d21*/
    goto LABEL_64; /*0x180001d21*/
  v58 = (_QWORD *)(a1 + 88520); /*0x180001d3c*/
  v43 = (*(__int64 (__fastcall **)(__int64 *, __int64, __int64, __int64, _DWORD, int, int *, _QWORD))(**v10 + 72))( /*0x180001d5f*/
          a1: *v10,
          a2: a1 + 88520,
          a3: 2,
          a4: 44100,
          a5: 0,
          a6: 1,
          a7: &v104,
          a8: 0);
  if ( v43 < 0 ) /*0x180001d63*/
    goto LABEL_61; /*0x180001d63*/
  sub_180002AC0(a1: &unk_180029254, a2: a1); /*0x180001d7b*/
  (*(void (__fastcall **)(_QWORD, _QWORD, __int64, __int64, _DWORD))(*(_QWORD *)*v57 + 48LL))( /*0x180001d96*/
    a1: *v57,
    a2: 0,
    a3: a1,
    a4: 52,
    a5: 0);
  *(_QWORD *)&v138[4] = *v57; /*0x180001d9c*/
  v66 = *v58; /*0x180001da3*/
  *(_DWORD *)v138 = 0; /*0x180001da6*/
  v140 = v66; /*0x180001dad*/
  v60 = (int *)v138; /*0x180001db4*/
  v139 = 0; /*0x180001dbb*/
LABEL_63:
  v67 = *v10; /*0x180001dc2*/
  v106 = 2; /*0x180001dd4*/
  v107 = v60; /*0x180001ddc*/
  v68 = (__int64 **)(a1 + 88504); /*0x180001de4*/
  v43 = (*(__int64 (__fastcall **)(__int64 *, __int64, __int64, __int64, _DWORD, int, int *, _QWORD))(*v67 + 72))( /*0x180001e0a*/
          a1: v67,
          a2: a1 + 88504,
          a3: 2,
          a4: 44100,
          a5: 0,
          a6: 1,
          a7: &v106,
          a8: 0);
  if ( v43 < 0 ) /*0x180001e0e*/
    goto LABEL_64; /*0x180001e0e*/
  v69 = *v68; /*0x180001e1c*/
  v70 = *v58; /*0x180001e20*/
  v71 = **v68; /*0x180001e2a*/
  if ( *(_DWORD *)(a1 + 88380) != 0 ) /*0x180001e47*/
  {
    v73 = (*(__int64 (__fastcall **)(__int64 *, __int64, __int64))(v71 + 128))(a1: v69, a2: v70, a3: 2); /*0x180002051*/
    if ( v73 >= 0 ) /*0x180002056*/
    {
      v75 = (*(__int64 (__fastcall **)(__int64 *, _QWORD, __int64, __int64, void *, _DWORD))(**v68 + 128))( /*0x180002091*/
              a1: *v68,
              a2: *v57,
              a3: 2,
              a4: 2,
              a5: &unk_180029040,
              a6: 0);
      if ( v75 >= 0 ) /*0x180002096*/
      {
        v75 = (*(__int64 (__fastcall **)(_QWORD, _QWORD, __int64, __int64, void *, _DWORD))(*(_QWORD *)*v57 + 128LL))( /*0x1800020c2*/
                a1: *v57,
                a2: *(_QWORD *)(a1 + 88488),
                a3: 2,
                a4: 2,
                a5: &unk_180029040,
                a6: 0);
        if ( v75 >= 0 ) /*0x1800020c7*/
        {
          v75 = (*(__int64 (__fastcall **)(_QWORD, _QWORD, __int64, __int64, void *, _DWORD))(*(_QWORD *)*v57 + 128LL))( /*0x1800020ef*/
                  a1: *v57,
                  a2: *(_QWORD *)(a1 + 88496),
                  a3: 2,
                  a4: 2,
                  a5: &unk_180029040,
                  a6: 0);
          if ( v75 >= 0 ) /*0x1800020f4*/
          {
            v75 = (*(__int64 (__fastcall **)(_QWORD, _QWORD, __int64, __int64, void *, _DWORD))(*(_QWORD *)*v57 + 128LL))( /*0x18000211c*/
                    a1: *v57,
                    a2: *(_QWORD *)(a1 + 88464),
                    a3: 2,
                    a4: 2,
                    a5: &unk_180029040,
                    a6: 0);
            if ( v75 >= 0 ) /*0x180002121*/
            {
              v77 = (*(__int64 (__fastcall **)(_QWORD, _QWORD, __int64, __int64, void *, _DWORD))(*(_QWORD *)*v57 + 128LL))( /*0x180002159*/
                      a1: *v57,
                      a2: *(_QWORD *)(a1 + 88472),
                      a3: 2,
                      a4: 2,
                      a5: &unk_180029040,
                      a6: 0);
              if ( v77 >= 0 ) /*0x18000215e*/
              {
                v77 = (*(__int64 (__fastcall **)(_QWORD, _QWORD, __int64, __int64, void *, _DWORD))(*(_QWORD *)*v58 /*0x180002183*/
                                                                                                  + 128LL))(
                        a1: *v58,
                        a2: *(_QWORD *)(a1 + 88488),
                        a3: 2,
                        a4: 2,
                        a5: &unk_180029040,
                        a6: 0);
                if ( v77 >= 0 ) /*0x180002188*/
                {
                  v77 = (*(__int64 (__fastcall **)(_QWORD, _QWORD, __int64, __int64, void *, _DWORD))(*(_QWORD *)*v58 + 128LL))( /*0x1800021ad*/
                          a1: *v58,
                          a2: *(_QWORD *)(a1 + 88496),
                          a3: 2,
                          a4: 2,
                          a5: &unk_180029040,
                          a6: 0);
                  if ( v77 >= 0 ) /*0x1800021b2*/
                  {
                    v77 = (*(__int64 (__fastcall **)(_QWORD, _QWORD, __int64, __int64, void *, _DWORD))(*(_QWORD *)*v58 + 128LL))( /*0x1800021d7*/
                            a1: *v58,
                            a2: *(_QWORD *)(a1 + 88464),
                            a3: 2,
                            a4: 2,
                            a5: &unk_180029040,
                            a6: 0);
                    if ( v77 >= 0 ) /*0x1800021dc*/
                    {
                      v79 = (*(__int64 (__fastcall **)(_QWORD, _QWORD, __int64, __int64, void *, _DWORD))(*(_QWORD *)*v58 + 128LL))( /*0x180002211*/
                              a1: *v58,
                              a2: *(_QWORD *)(a1 + 88472),
                              a3: 2,
                              a4: 2,
                              a5: &unk_180029040,
                              a6: 0);
                      if ( v79 >= 0 ) /*0x180002215*/
                      {
                        v80 = (_QWORD *)(a1 + 88408); /*0x180002222*/
                        v79 = (*(__int64 (__fastcall **)(_QWORD, _QWORD, __int64, __int64, void *, _DWORD))(**(_QWORD **)(a1 + 88528) + 128LL))( /*0x180002245*/
                                a1: *(_QWORD *)(a1 + 88528),
                                a2: *(_QWORD *)(a1 + 88408),
                                a3: 2,
                                a4: 2,
                                a5: &unk_180029040,
                                a6: 0);
                        if ( v79 >= 0 ) /*0x180002249*/
                        {
                          v79 = (*(__int64 (__fastcall **)(_QWORD, _QWORD, __int64, __int64, void *, _DWORD))(**(_QWORD **)(a1 + 88424) + 128LL))( /*0x180002272*/
                                  a1: *(_QWORD *)(a1 + 88424),
                                  a2: *v80,
                                  a3: 2,
                                  a4: 2,
                                  a5: &unk_180029040,
                                  a6: 0);
                          if ( v79 >= 0 ) /*0x180002276*/
                          {
                            v79 = (*(__int64 (__fastcall **)(_QWORD, _QWORD, __int64, __int64, void *, _DWORD))(**(_QWORD **)(a1 + 88488) + 128LL))( /*0x1800022aa*/
                                    a1: *(_QWORD *)(a1 + 88488),
                                    a2: *(_QWORD *)(a1 + 88440),
                                    a3: 2,
                                    a4: 2,
                                    a5: &unk_180029050,
                                    a6: 0);
                            if ( v79 >= 0 ) /*0x1800022ae*/
                            {
                              v79 = (*(__int64 (__fastcall **)(_QWORD, _QWORD, __int64, __int64, void *, _DWORD))(**(_QWORD **)(a1 + 88496) + 128LL))( /*0x1800022e2*/
                                      a1: *(_QWORD *)(a1 + 88496),
                                      a2: *(_QWORD *)(a1 + 88440),
                                      a3: 2,
                                      a4: 2,
                                      a5: &unk_180029060,
                                      a6: 0);
                              if ( v79 >= 0 ) /*0x1800022e6*/
                              {
                                v79 = (*(__int64 (__fastcall **)(_QWORD, _QWORD, __int64, __int64, void *, _DWORD))(**(_QWORD **)(a1 + 88464) + 128LL))( /*0x18000231a*/
                                        a1: *(_QWORD *)(a1 + 88464),
                                        a2: *(_QWORD *)(a1 + 88440),
                                        a3: 2,
                                        a4: 2,
                                        a5: &unk_180029050,
                                        a6: 0);
                                if ( v79 >= 0 ) /*0x18000231e*/
                                {
                                  v79 = (*(__int64 (__fastcall **)(_QWORD, _QWORD, __int64, __int64, void *, _DWORD))(**(_QWORD **)(a1 + 88472) + 128LL))( /*0x18000234e*/
                                          a1: *(_QWORD *)(a1 + 88472),
                                          a2: *(_QWORD *)(a1 + 88440),
                                          a3: 2,
                                          a4: 2,
                                          a5: &unk_180029060,
                                          a6: 0);
                                  if ( v79 >= 0 ) /*0x180002352*/
                                  {
                                    v79 = (*(__int64 (__fastcall **)(_QWORD, _QWORD, __int64, __int64, void *, _DWORD))(**(_QWORD **)(a1 + 88440) + 128LL))( /*0x18000237b*/
                                            a1: *(_QWORD *)(a1 + 88440),
                                            a2: *(_QWORD *)(a1 + 88448),
                                            a3: 2,
                                            a4: 2,
                                            a5: &unk_180029040,
                                            a6: 0);
                                    if ( v79 >= 0 ) /*0x18000237f*/
                                    {
                                      v43 = (*(__int64 (__fastcall **)(_QWORD, _QWORD, __int64, __int64, void *, _DWORD))(**(_QWORD **)(a1 + 88448) + 128LL))( /*0x1800023b0*/
                                              a1: *(_QWORD *)(a1 + 88448),
                                              a2: *v80,
                                              a3: 2,
                                              a4: 2,
                                              a5: &unk_180029040,
                                              a6: 0);
                                      if ( v43 < 0 ) /*0x1800023b4*/
                                        goto LABEL_77; /*0x1800023b4*/
                                      goto LABEL_100; /*0x1800023b4*/
                                    }
                                  }
                                }
                              }
                            }
                          }
                        }
LABEL_98:
                        v82 = &unk_18001B868; /*0x180002381*/
LABEL_114:
                        avs2_core_380(a1: "Class_XAudioControl::Init", a2: v82, a3: v81); /*0x18000250b*/
                      }
LABEL_115:
                      sub_1800027D0(a1, a2: v78); /*0x180002518*/
                      return (unsigned int)v79; /*0x180002522*/
                    }
                  }
                }
              }
LABEL_89:
              sub_1800027D0(a1, a2: v76); /*0x1800021de*/
              return (unsigned int)v77; /*0x1800021e9*/
            }
          }
        }
      }
LABEL_84:
      sub_1800027D0(a1, a2: v74); /*0x180002123*/
      return (unsigned int)v75; /*0x18000212e*/
    }
LABEL_79:
    sub_1800027D0(a1, a2: v72); /*0x180002058*/
    return (unsigned int)v73; /*0x180002063*/
  }
  v73 = (*(__int64 (__fastcall **)(__int64 *, __int64, __int64))(v71 + 128))(a1: v69, a2: v70, a3: 2); /*0x180001e53*/
  if ( v73 < 0 ) /*0x180001e58*/
    goto LABEL_79; /*0x180001e58*/
  v75 = (*(__int64 (__fastcall **)(__int64 *, _QWORD, __int64, __int64, void *, _DWORD))(**v68 + 128))( /*0x180001e87*/
          a1: *v68,
          a2: *v57,
          a3: 2,
          a4: 2,
          a5: &unk_180029040,
          a6: 0);
  if ( v75 < 0 ) /*0x180001e8c*/
    goto LABEL_84; /*0x180001e8c*/
  v75 = (*(__int64 (__fastcall **)(_QWORD, _QWORD, __int64, __int64, void *, _DWORD))(*(_QWORD *)*v57 + 128LL))( /*0x180001eb8*/
          a1: *v57,
          a2: *(_QWORD *)(a1 + 88480),
          a3: 2,
          a4: 2,
          a5: &unk_180029040,
          a6: 0);
  if ( v75 < 0 ) /*0x180001ebd*/
    goto LABEL_84; /*0x180001ebd*/
  v77 = (*(__int64 (__fastcall **)(_QWORD, _QWORD, __int64, __int64, void *, _DWORD))(*(_QWORD *)*v57 + 128LL))( /*0x180001ee9*/
          a1: *v57,
          a2: *(_QWORD *)(a1 + 88456),
          a3: 2,
          a4: 2,
          a5: &unk_180029040,
          a6: 0);
  if ( v77 < 0 ) /*0x180001eee*/
    goto LABEL_89; /*0x180001eee*/
  v77 = (*(__int64 (__fastcall **)(_QWORD, _QWORD, __int64, __int64, void *, _DWORD))(*(_QWORD *)*v58 + 128LL))( /*0x180001f17*/
          a1: *v58,
          a2: *(_QWORD *)(a1 + 88480),
          a3: 2,
          a4: 2,
          a5: &unk_180029040,
          a6: 0);
  if ( v77 < 0 ) /*0x180001f1c*/
    goto LABEL_89; /*0x180001f1c*/
  v79 = (*(__int64 (__fastcall **)(_QWORD, _QWORD, __int64, __int64, void *, _DWORD))(*(_QWORD *)*v58 + 128LL))( /*0x180001f45*/
          a1: *v58,
          a2: *(_QWORD *)(a1 + 88456),
          a3: 2,
          a4: 2,
          a5: &unk_180029040,
          a6: 0);
  if ( v79 < 0 ) /*0x180001f49*/
    goto LABEL_115; /*0x180001f49*/
  v80 = (_QWORD *)(a1 + 88408); /*0x180001f56*/
  v79 = (*(__int64 (__fastcall **)(_QWORD, _QWORD, __int64, __int64, void *, _DWORD))(**(_QWORD **)(a1 + 88528) + 128LL))( /*0x180001f79*/
          a1: *(_QWORD *)(a1 + 88528),
          a2: *(_QWORD *)(a1 + 88408),
          a3: 2,
          a4: 2,
          a5: &unk_180029040,
          a6: 0);
  if ( v79 < 0 ) /*0x180001f7d*/
    goto LABEL_98; /*0x180001f7d*/
  v79 = (*(__int64 (__fastcall **)(_QWORD, _QWORD, __int64, __int64, void *, _DWORD))(**(_QWORD **)(a1 + 88424) + 128LL))( /*0x180001fa6*/
          a1: *(_QWORD *)(a1 + 88424),
          a2: *v80,
          a3: 2,
          a4: 2,
          a5: &unk_180029040,
          a6: 0);
  if ( v79 < 0 ) /*0x180001faa*/
    goto LABEL_98; /*0x180001faa*/
  v79 = (*(__int64 (__fastcall **)(_QWORD, _QWORD, __int64, __int64, void *, _DWORD))(**(_QWORD **)(a1 + 88480) + 128LL))( /*0x180001fd7*/
          a1: *(_QWORD *)(a1 + 88480),
          a2: *(_QWORD *)(a1 + 88432),
          a3: 2,
          a4: 2,
          a5: &unk_180029040,
          a6: 0);
  if ( v79 < 0 ) /*0x180001fdb*/
    goto LABEL_98; /*0x180001fdb*/
  v79 = (*(__int64 (__fastcall **)(_QWORD, _QWORD, __int64, __int64, void *, _DWORD))(**(_QWORD **)(a1 + 88456) + 128LL))( /*0x180002008*/
          a1: *(_QWORD *)(a1 + 88456),
          a2: *(_QWORD *)(a1 + 88432),
          a3: 2,
          a4: 2,
          a5: &unk_180029040,
          a6: 0);
  if ( v79 < 0 ) /*0x18000200c*/
    goto LABEL_98; /*0x18000200c*/
  v43 = (*(__int64 (__fastcall **)(_QWORD, _QWORD, __int64, __int64, void *, _DWORD))(**(_QWORD **)(a1 + 88432) + 128LL))( /*0x180002035*/
          a1: *(_QWORD *)(a1 + 88432),
          a2: *v80,
          a3: 2,
          a4: 2,
          a5: &unk_180029040,
          a6: 0);
  if ( v43 < 0 ) /*0x180002039*/
  {
LABEL_77:
    v52 = &unk_18001B868; /*0x18000203f*/
    goto LABEL_118; /*0x180002046*/
  }
LABEL_100:
  if ( v129 == 2 ) /*0x1800023d2*/
  {
    v83 = (*(__int64 (__fastcall **)(_QWORD, _QWORD, __int64, __int64, void *, _DWORD))(*(_QWORD *)*v80 + 128LL))( /*0x180002405*/
            a1: *v80,
            a2: *(_QWORD *)(a1 + 88400),
            a3: 2,
            a4: 2,
            a5: &unk_180029000,
            a6: 0);
  }
  else
  {
    if ( v129 != 6 ) /*0x1800023d7*/
      goto LABEL_105; /*0x1800023d7*/
    v83 = (*(__int64 (__fastcall **)(_QWORD, _QWORD, __int64, __int64, void *, _DWORD))(*(_QWORD *)*v80 + 128LL))( /*0x1800023e6*/
            a1: *v80,
            a2: *(_QWORD *)(a1 + 88400),
            a3: 2,
            a4: 6,
            a5: &unk_180029010,
            a6: 0);
  }
  v43 = v83; /*0x180002412*/
LABEL_105:
  if ( v43 < 0 ) /*0x180002416*/
    goto LABEL_77; /*0x180002416*/
  if ( v129 == 2 ) /*0x180002426*/
  {
    v84 = (*(__int64 (__fastcall **)(_QWORD, _QWORD, __int64, __int64, void *, _DWORD))(**(_QWORD **)(a1 + 88416) + 128LL))( /*0x18000245d*/
            a1: *(_QWORD *)(a1 + 88416),
            a2: *(_QWORD *)(a1 + 88400),
            a3: 2,
            a4: 2,
            a5: &unk_180029000,
            a6: 0);
  }
  else
  {
    if ( v129 != 6 ) /*0x18000242b*/
      goto LABEL_111; /*0x18000242b*/
    v84 = (*(__int64 (__fastcall **)(_QWORD, _QWORD, __int64, __int64, void *, _DWORD))(**(_QWORD **)(a1 + 88416) + 128LL))( /*0x18000243a*/
            a1: *(_QWORD *)(a1 + 88416),
            a2: *(_QWORD *)(a1 + 88400),
            a3: 2,
            a4: 6,
            a5: &unk_180029010,
            a6: 0);
  }
  v43 = v84; /*0x180002463*/
LABEL_111:
  if ( v43 < 0 ) /*0x180002467*/
    goto LABEL_77; /*0x180002467*/
  avs2_core_244(a1: a1 + 88324, a2: 0, a3: 44); /*0x18000247a*/
  v85 = *v10; /*0x180002480*/
  *(_DWORD *)(a1 + 88328) = 88200; /*0x180002494*/
  *(_DWORD *)(a1 + 88356) = 255; /*0x18000249e*/
  *(_DWORD *)(a1 + 88324) = 64; /*0x1800024a8*/
  v126[0] = 65537; /*0x1800024b2*/
  v126[1] = 44100; /*0x1800024b9*/
  *(_QWORD *)(a1 + 88332) = a1 + 124; /*0x1800024c0*/
  v126[2] = 88200; /*0x1800024c7*/
  v126[3] = 1048578; /*0x1800024ce*/
  v127 = 0; /*0x1800024d5*/
  v79 = (*(__int64 (__fastcall **)(__int64 *, __int64, _DWORD *, _QWORD, int, _QWORD, _QWORD, _QWORD))(*v85 + 64))( /*0x1800024fe*/
          a1: v85,
          a2: a1 + 88368,
          a3: v126,
          a4: 0,
          a5: 0x40000000,
          a6: 0,
          a7: 0,
          a8: 0);
  if ( v79 < 0 ) /*0x180002502*/
  {
    v82 = &unk_18001B898; /*0x180002504*/
    goto LABEL_114; /*0x180002504*/
  }
  (*(void (__fastcall **)(_QWORD, __int64, _QWORD))(**(_QWORD **)(a1 + 88368) + 168LL))( /*0x18000253b*/
    a1: *(_QWORD *)(a1 + 88368),
    a2: a1 + 88324,
    a3: 0);
  (*(void (__fastcall **)(_QWORD, _QWORD, _QWORD))(**(_QWORD **)(a1 + 88368) + 152LL))( /*0x180002550*/
    a1: *(_QWORD *)(a1 + 88368),
    a2: 0,
    a3: 0);
  v86 = (__int64 **)(a1 + 88384); /*0x180002556*/
  v43 = sub_1800035C0(a1: 0, a2: a1 + 88384); /*0x180002567*/
  if ( v43 < 0 ) /*0x18000256b*/
  {
    v52 = &unk_18001B8D0; /*0x18000256d*/
    goto LABEL_118; /*0x18000256d*/
  }
  memset(Buffer, 0, 260); /*0x18000259f*/
  GetCurrentDirectoryA(nBufferLength: 0x104u, lpBuffer: Buffer); /*0x1800025b7*/
  v88 = avs2_core_79(a1: a2, a2: 1, a3: 511); /*0x1800025d4*/
  *(_DWORD *)(a1 + 88600) = v88; /*0x1800025da*/
  if ( v88 > 0 ) /*0x1800025e2*/
  {
    v90 = sub_180003840(a1: (unsigned int)v88); /*0x18000260f*/
    *(_DWORD *)(a1 + 88604) = v90; /*0x180002614*/
    if ( v90 == -1 ) /*0x18000261d*/
    {
      sub_1800027D0(a1, a2: v91); /*0x180002622*/
      return 2147500037LL; /*0x180002627*/
    }
    else
    {
      v92 = CoTaskMemAlloc(cb: v90); /*0x180002633*/
      *(_QWORD *)(a1 + 88608) = v92; /*0x180002639*/
      if ( v92 != nullptr ) /*0x180002643*/
      {
        if ( (int)avs2_core_82(a1: *(unsigned int *)(a1 + 88600), a2: v92, a3: *(unsigned int *)(a1 + 88604)) >= 0 ) /*0x18000266f*/
        {
          v95 = *(unsigned int *)(a1 + 88600); /*0x180002683*/
          if ( (int)v95 > 0 ) /*0x18000268b*/
          {
            avs2_core_86(a1: v95); /*0x18000268d*/
            *(_DWORD *)(a1 + 88600) = 0; /*0x180002693*/
          }
          v96 = *v86; /*0x18000269a*/
          v120 = 1; /*0x1800026a4*/
          v109 = (__int64)DllMain; /*0x1800026a8*/
          v123 = 0; /*0x1800026b4*/
          v122 = DllMain; /*0x1800026be*/
          v108 = sub_180003880; /*0x1800026c3*/
          v97 = *(_QWORD *)(a1 + 88608); /*0x1800026c8*/
          v117 = 250; /*0x1800026cf*/
          v121 = sub_180003880; /*0x1800026dc*/
          v118 = v97; /*0x1800026e1*/
          v119 = *(_DWORD *)(a1 + 88604); /*0x1800026ef*/
          v124 = *v10; /*0x1800026f5*/
          v125 = *(_QWORD *)(a1 + 88400); /*0x180002700*/
          v43 = (*(__int64 (__fastcall **)(__int64 *, int *))(*v96 + 48))(a1: v96, a2: &v117); /*0x18000270a*/
          if ( v43 < 0 ) /*0x18000270e*/
            goto LABEL_119; /*0x18000270e*/
          v98 = &unk_180038838; /*0x180002714*/
          for ( i = 1024; i != 0; --i ) /*0x18000271b*/
          {
            *(v98 - 1) = 0; /*0x180002725*/
            *v98 = 0; /*0x180002729*/
            v98[1] = 0; /*0x18000272c*/
            v98[2] = 0; /*0x180002730*/
            v98[3] = 0; /*0x180002734*/
            v98[4] = 0; /*0x180002738*/
            v98[5] = 0; /*0x18000273c*/
            sub_180005A30(a1: v98); /*0x180002740*/
            v98 += 7; /*0x180002745*/
          }
          v100 = *v86; /*0x18000274e*/
          if ( *v86 != nullptr ) /*0x180002754*/
          {
            v101 = *v100; /*0x180002756*/
            (*(void (__fastcall **)(__int64 *, const char *))(*v100 + 136))(a1: v100, a2: "Global"); /*0x180002760*/
            (*(void (__fastcall **)(__int64 *, _QWORD))(v101 + 152))(a1: *v86, a2: v102); /*0x180002774*/
          }
          return 0; /*0x18000277a*/
        }
        else
        {
          sub_1800027D0(a1, a2: v94); /*0x180002674*/
          return 2147549183LL; /*0x180002679*/
        }
      }
      else
      {
        sub_1800027D0(a1, a2: v93); /*0x180002648*/
        return 2147942414LL; /*0x18000264d*/
      }
    }
  }
  else
  {
    avs2_core_380(a1: "Class_XAudioControl::Init", a2: &unk_18001B8F8, a3: a2); /*0x1800025f5*/
    sub_1800027D0(a1, a2: v89); /*0x1800025fe*/
    return 2147942406LL; /*0x180002603*/
  }
}
