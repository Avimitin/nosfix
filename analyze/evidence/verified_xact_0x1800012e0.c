// nosfix analysis 2026-10-29: Creates XAudio2 engine/master/submix/effects and looping 44.1 kHz mono 16-bit clock voice; supplies XAudio2/master voice to native XACT initialization. ASIO engine substitution candidate, compatibility untested.
__int64 __fastcall sub_1800012E0(__int64 a1, __int64 a2, int a3)
{
  __int64 v6; // rdx
  __int64 **v8; // r14
  int v9; // ebx
  __int64 v10; // rdx
  __int64 (__fastcall **v11)(int, int, int, int, __int64); // r15
  HRESULT Instance; // ebx
  __int64 v13; // rdx
  __int64 v14; // rdx
  int FX; // esi
  __int64 v16; // rdx
  __int64 v17; // rcx
  float v18; // xmm0_4
  __int64 v19; // r8
  __int64 v20; // rdx
  int v21; // ebx
  __int64 v22; // rdx
  __int64 *v23; // rcx
  __int64 *v24; // rcx
  __int64 v25; // rax
  int v26; // r8d
  int v27; // r9d
  int v28; // xmm0_4
  int v29; // edx
  __int64 *v30; // rcx
  int v31; // r8d
  int v32; // r9d
  __int64 *v33; // rcx
  _BYTE *v34; // rax
  __int64 v35; // rax
  int v36; // r8d
  int v37; // r9d
  __int64 v38; // rdx
  int *v39; // rax
  __int64 v40; // rax
  __int64 (__fastcall *v41)(int, int, int, int, __int64); // rax
  int v42; // r8d
  int v43; // r9d
  int v44; // r8d
  int v45; // r9d
  __int64 *v46; // rcx
  __int64 v47; // rax
  __int64 v48; // rax
  __int64 (__fastcall *v49)(int, int, int, int, __int64); // rax
  _QWORD *v50; // r15
  _QWORD *v51; // rsi
  __int64 v52; // rax
  int *v53; // rax
  _BYTE *v54; // rax
  __int64 v55; // rax
  __int64 v56; // rax
  __int64 v57; // rax
  __int64 (__fastcall *v58)(int, int, int, int, __int64); // rax
  __int64 v59; // rax
  __int64 *v60; // rcx
  __int64 **v61; // r12
  __int64 *v62; // rcx
  __int64 v63; // rdx
  __int64 v64; // rax
  __int64 v65; // rdx
  int v66; // r13d
  __int64 v67; // rdx
  int v68; // r12d
  __int64 v69; // rdx
  int v70; // r15d
  __int64 v71; // rdx
  int v72; // esi
  _QWORD *v73; // r15
  int v74; // eax
  int v75; // eax
  __int64 *v76; // rcx
  __int64 **v77; // rsi
  int v78; // eax
  __int64 v79; // rdx
  unsigned int v80; // eax
  __int64 v81; // rdx
  LPVOID v82; // rax
  __int64 v83; // rdx
  __int64 v84; // rdx
  __int64 v85; // rcx
  __int64 *v86; // rcx
  __int64 v87; // rax
  _QWORD *v88; // rbx
  __int64 i; // rdi
  __int64 *v90; // rcx
  __int64 v91; // rbx
  unsigned __int16 v92; // ax
  int v94; // [rsp+48h] [rbp-B8h] BYREF
  int *v95; // [rsp+4Ch] [rbp-B4h]
  int v96; // [rsp+58h] [rbp-A8h] BYREF
  int *v97; // [rsp+5Ch] [rbp-A4h]
  __int64 (__fastcall *v98)(int, int, int, int, __int64); // [rsp+68h] [rbp-98h] BYREF
  __int64 v99; // [rsp+70h] [rbp-90h]
  _BYTE v100[12]; // [rsp+78h] [rbp-88h] BYREF
  int v101; // [rsp+84h] [rbp-7Ch]
  __int64 v102; // [rsp+88h] [rbp-78h] BYREF
  int v103; // [rsp+90h] [rbp-70h]
  int v104; // [rsp+94h] [rbp-6Ch]
  int v105; // [rsp+98h] [rbp-68h] BYREF
  __int64 *v106; // [rsp+9Ch] [rbp-64h]
  int v107; // [rsp+B0h] [rbp-50h] BYREF
  __int64 v108; // [rsp+B8h] [rbp-48h]
  int v109; // [rsp+C0h] [rbp-40h]
  int v110; // [rsp+C4h] [rbp-3Ch]
  __int64 (__fastcall *v111)(int, int, int, int, __int64); // [rsp+D0h] [rbp-30h]
  BOOL (__stdcall *v112)(HINSTANCE, DWORD, LPVOID); // [rsp+D8h] [rbp-28h]
  __int64 v113; // [rsp+E0h] [rbp-20h]
  __int64 *v114; // [rsp+F0h] [rbp-10h]
  __int64 v115; // [rsp+F8h] [rbp-8h]
  _DWORD v116[4]; // [rsp+100h] [rbp+0h] BYREF
  __int16 v117; // [rsp+110h] [rbp+10h]
  char v118[1030]; // [rsp+120h] [rbp+20h] BYREF
  unsigned __int16 v119; // [rsp+526h] [rbp+426h]
  int v120; // [rsp+550h] [rbp+450h] BYREF
  _BYTE *v121; // [rsp+554h] [rbp+454h]
  int v122; // [rsp+55Ch] [rbp+45Ch]
  __int64 v123; // [rsp+560h] [rbp+460h]
  int v124; // [rsp+568h] [rbp+468h]
  __int64 v125; // [rsp+56Ch] [rbp+46Ch]
  int v126; // [rsp+574h] [rbp+474h]
  __int64 v127; // [rsp+578h] [rbp+478h]
  _BYTE v128[12]; // [rsp+580h] [rbp+480h] BYREF
  int v129; // [rsp+58Ch] [rbp+48Ch]
  __int64 v130; // [rsp+590h] [rbp+490h]
  CHAR Buffer[272]; // [rsp+5A0h] [rbp+4A0h] BYREF

  avs2_core_244(a1: &v107, a2: 0, a3: 80); /*0x18000131d*/
  if ( a2 == 0 ) /*0x180001326*/
  {
    avs2_core_380(a1: "Class_XAudioControl::Init", a2: aXgs); /*0x180001336*/
    return 2147942487LL; /*0x180001341*/
  }
  sub_1800027D0(a1, a2: v6); /*0x180001349*/
  *(_DWORD *)(a1 + 88380) = a3; /*0x180001360*/
  if ( (int)avs2_core_53(a1: &unk_180052D30, a2: 150000000, a3: 64) < 0 ) /*0x18000136e*/
  {
    avs2_core_380(a1: "Class_XAudioControl::Init", a2: aHeapBoot); /*0x180001377*/
    return 2147942487LL; /*0x180001377*/
  }
  v8 = (__int64 **)(a1 + 88392); /*0x180001381*/
  v9 = sub_180003730(a1: a1 + 88392, a2: 0, a3: 0xFFFFFFFFLL); /*0x180001396*/
  if ( v9 < 0 ) /*0x18000139a*/
  {
    avs2_core_380(a1: "Class_XAudioControl::Init", a2: aXaudio2create); /*0x1800013aa*/
    sub_1800027D0(a1, a2: v10); /*0x1800013b3*/
    return (unsigned int)v9; /*0x1800013ba*/
  }
  (*(void (__fastcall **)(__int64 *, _QWORD, char *))(**v8 + 32))(a1: *v8, a2: 0, a3: v118); /*0x1800013db*/
  v11 = (__int64 (__fastcall **)(int, int, int, int, __int64))(a1 + 88560); /*0x1800013de*/
  Instance = CoCreateInstance( /*0x180001409*/
               rclsid: &rclsid,
               pUnkOuter: nullptr,
               dwClsContext: 1u,
               riid: &riid,
               ppv: (LPVOID *)(a1 + 88560));
  if ( Instance < 0 ) /*0x18000140d*/
  {
    avs2_core_380(a1: "Class_XAudioControl::Init", a2: aXaudio2); /*0x180001416*/
LABEL_14:
    sub_1800027D0(a1, a2: v13); /*0x180001457*/
    return (unsigned int)Instance; /*0x180001461*/
  }
  if ( *(_DWORD *)(a1 + 88380) != 0 ) /*0x180001426*/
  {
    Instance = CreateFX(a1: &unk_18001B498, a2: a1 + 88576); /*0x18000143d*/
    if ( Instance < 0 ) /*0x180001441*/
    {
      avs2_core_380(a1: "Class_XAudioControl::Init", a2: aXaudio2_0); /*0x180001451*/
      goto LABEL_14; /*0x180001451*/
    }
    v14 = a1 + 88584; /*0x180001466*/
  }
  else
  {
    v14 = a1 + 88568; /*0x180001428*/
  }
  FX = CreateFX(a1: &unk_18001B498, a2: v14); /*0x180001479*/
  if ( FX < 0 ) /*0x18000147d*/
  {
    avs2_core_380(a1: "Class_XAudioControl::Init", a2: aXaudio2_0); /*0x18000148d*/
    sub_1800027D0(a1, a2: v16); /*0x180001496*/
    return (unsigned int)FX; /*0x18000149d*/
  }
  avs2_core_244(a1: a1 + 52, a2: 0, a3: 12); /*0x1800014bc*/
  avs2_core_244(a1: a1 + 64, a2: 0, a3: 12); /*0x1800014cc*/
  avs2_core_244(a1: a1 + 76, a2: 0, a3: 12); /*0x1800014dc*/
  *(_DWORD *)(a1 + 88) = 0; /*0x1800014f2*/
  v18 = sub_1800037D0(a1: v17, a2: 48000); /*0x1800014f9*/
  *(_QWORD *)(a1 + 96) = 1065353216; /*0x18000150c*/
  *(_QWORD *)(a1 + 108) = 1065353216; /*0x180001514*/
  *(float *)(a1 + 92) = v18; /*0x18000151c*/
  *(float *)(a1 + 104) = v18; /*0x180001521*/
  *(float *)(a1 + 116) = v18; /*0x180001526*/
  *(_DWORD *)(a1 + 120) = 1065353216; /*0x18000152b*/
  CreateFX(a1: &unk_18001B4A8, a2: a1 + 88592); /*0x180001532*/
  v102 = *(_QWORD *)(a1 + 88592); /*0x18000153e*/
  v103 = 0; /*0x18000154d*/
  v105 = 1; /*0x180001551*/
  v106 = &v102; /*0x180001555*/
  v104 = v119; /*0x180001559*/
  if ( v119 == 2 ) /*0x18000155f*/
  {
    v19 = 2; /*0x18000156b*/
  }
  else
  {
    if ( v119 != 6 ) /*0x180001564*/
      goto LABEL_23; /*0x180001564*/
    v19 = 6; /*0x180001566*/
  }
  FX = (*(__int64 (__fastcall **)(__int64 *, __int64, __int64, __int64, _DWORD, _DWORD, _QWORD))(**v8 + 80))( /*0x180001596*/
         a1: *v8,
         a2: a1 + 88400,
         a3: v19,
         a4: 44100,
         a5: 0,
         a6: 0,
         a7: 0);
LABEL_23:
  if ( FX < 0 ) /*0x18000159a*/
  {
    avs2_core_380(a1: "Class_XAudioControl::Init", a2: aXaudio2_1); /*0x1800015aa*/
    sub_1800027D0(a1, a2: v20); /*0x1800015b3*/
    return (unsigned int)FX; /*0x1800015ba*/
  }
  v21 = (*(__int64 (__fastcall **)(_QWORD, int *))(**(_QWORD **)(a1 + 88400) + 16LL))( /*0x1800015d0*/
          a1: *(_QWORD *)(a1 + 88400),
          a2: &v105);
  if ( v21 < 0 ) /*0x1800015d4*/
  {
    avs2_core_380(a1: "Class_XAudioControl::Init", a2: aXaudio2_2); /*0x1800015dd*/
LABEL_43:
    sub_1800027D0(a1, a2: v22); /*0x180001873*/
    return (unsigned int)v21; /*0x18000187d*/
  }
  v23 = *v8; /*0x1800015e9*/
  v121 = *(_BYTE **)(a1 + 88400); /*0x1800015f1*/
  *(_DWORD *)(a1 + 88616) = 0; /*0x180001609*/
  v120 = 0; /*0x180001610*/
  v95 = &v120; /*0x180001617*/
  v94 = 1; /*0x18000161c*/
  v21 = (*(__int64 (__fastcall **)(__int64 *, __int64, __int64, __int64, _DWORD, int, int *, _QWORD))(*v23 + 72))( /*0x180001647*/
          a1: v23,
          a2: a1 + 88408,
          a3: 2,
          a4: 44100,
          a5: 0,
          a6: 5,
          a7: &v94,
          a8: 0);
  if ( v21 < 0 ) /*0x18000164b*/
  {
    avs2_core_380(a1: "Class_XAudioControl::Init", a2: aXaudio2_3); /*0x180001654*/
    goto LABEL_43; /*0x180001654*/
  }
  v21 = (*(__int64 (__fastcall **)(__int64 *, __int64, __int64, __int64, _DWORD, int, int *, _QWORD))(**v8 + 72))( /*0x180001691*/
          a1: *v8,
          a2: a1 + 88416,
          a3: 2,
          a4: 44100,
          a5: 0,
          a6: 5,
          a7: &v94,
          a8: 0);
  if ( v21 < 0 ) /*0x180001695*/
  {
    avs2_core_380(a1: "Class_XAudioControl::Init", a2: aXaudio2_4); /*0x18000169e*/
    goto LABEL_43; /*0x18000169e*/
  }
  v24 = *v8; /*0x1800016aa*/
  v121 = *(_BYTE **)(a1 + 88408); /*0x1800016b2*/
  v120 = 0; /*0x1800016ca*/
  v94 = 1; /*0x1800016d1*/
  v95 = &v120; /*0x1800016d6*/
  v25 = *v24; /*0x1800016db*/
  if ( *(_DWORD *)(a1 + 88380) == 0 ) /*0x180001701*/
  {
    v21 = (*(__int64 (__fastcall **)(__int64 *, __int64, __int64, __int64, int, int, int *, _QWORD))(v25 + 72))( /*0x18000170d*/
            a1: v24,
            a2: a1 + 88432,
            a3: 2,
            a4: 44100,
            a5: 8,
            a6: 4,
            a7: &v94,
            a8: 0);
    if ( v21 >= 0 ) /*0x180001711*/
    {
      v28 = *(_DWORD *)(a1 + 96); /*0x18000171c*/
      v29 = 0; /*0x180001721*/
      goto LABEL_36; /*0x180001723*/
    }
LABEL_42:
    avs2_core_380(a1: "Class_XAudioControl::Init", a2: aXaudio2_5); /*0x18000185f*/
    goto LABEL_43; /*0x18000186d*/
  }
  v21 = (*(__int64 (__fastcall **)(__int64 *, __int64, __int64, __int64, int, int, int *, _QWORD))(v25 + 72))( /*0x18000172f*/
          a1: v24,
          a2: a1 + 88448,
          a3: 2,
          a4: 44100,
          a5: 8,
          a6: 4,
          a7: &v94,
          a8: 0);
  if ( v21 < 0 ) /*0x180001733*/
    goto LABEL_42; /*0x180001733*/
  v28 = *(_DWORD *)(a1 + 120); /*0x18000173e*/
  v29 = 1; /*0x180001743*/
LABEL_36:
  sub_180002F40(a1, a2: v29, a3: v26, a4: v27, a5: v28); /*0x180001746*/
  v21 = (*(__int64 (__fastcall **)(__int64 *, __int64, __int64, __int64, _DWORD, int, int *, _QWORD))(**v8 + 72))( /*0x180001792*/
          a1: *v8,
          a2: a1 + 88528,
          a3: 2,
          a4: 44100,
          a5: 0,
          a6: 4,
          a7: &v94,
          a8: 0);
  if ( v21 < 0 ) /*0x180001796*/
  {
    avs2_core_380(a1: "Class_XAudioControl::Init", a2: aXaudio2_6); /*0x18000179f*/
    goto LABEL_43; /*0x18000179f*/
  }
  v21 = (*(__int64 (__fastcall **)(__int64 *, __int64, __int64, __int64, _DWORD, int, int *, _QWORD))(**v8 + 72))( /*0x1800017dc*/
          a1: *v8,
          a2: a1 + 88424,
          a3: 2,
          a4: 44100,
          a5: 0,
          a6: 4,
          a7: &v94,
          a8: 0);
  if ( v21 < 0 ) /*0x1800017e0*/
  {
    avs2_core_380(a1: "Class_XAudioControl::Init", a2: aXaudio2_7); /*0x1800017e9*/
    goto LABEL_43; /*0x1800017e9*/
  }
  if ( *(_DWORD *)(a1 + 88380) != 0 ) /*0x1800017f2*/
  {
    v30 = *v8; /*0x1800017ff*/
    v121 = *(_BYTE **)(a1 + 88448); /*0x180001807*/
    v120 = 0; /*0x18000181f*/
    v94 = 1; /*0x180001826*/
    v95 = &v120; /*0x18000182b*/
    v21 = (*(__int64 (__fastcall **)(__int64 *, __int64, __int64, __int64, int, int, int *, _QWORD))(*v30 + 72))( /*0x180001859*/
            a1: v30,
            a2: a1 + 88440,
            a3: 2,
            a4: 44100,
            a5: 8,
            a6: 3,
            a7: &v94,
            a8: 0);
    if ( v21 < 0 ) /*0x18000185d*/
      goto LABEL_42; /*0x18000185d*/
    sub_180002F40(a1, a2: 0, a3: v31, a4: v32, a5: *(_DWORD *)(a1 + 108)); /*0x18000189d*/
  }
  v33 = *v8; /*0x1800018a2*/
  v94 = 1; /*0x1800018ad*/
  if ( *(_DWORD *)(a1 + 88380) == 0 ) /*0x1800018bf*/
  {
    v34 = *(_BYTE **)(a1 + 88432); /*0x1800018c5*/
    v120 = 0; /*0x1800018d1*/
    v121 = v34; /*0x1800018dd*/
    *(_DWORD *)&v100[8] = 1; /*0x1800018f5*/
    v95 = &v120; /*0x1800018f9*/
    v35 = *(_QWORD *)(a1 + 88568); /*0x1800018fe*/
    v101 = 2; /*0x180001905*/
    *(_QWORD *)v100 = v35; /*0x18000190c*/
    v96 = 1; /*0x180001916*/
    v97 = (int *)v100; /*0x18000191b*/
    v21 = (*(__int64 (__fastcall **)(__int64 *, __int64, __int64, __int64, _DWORD, int, int *, int *))(*v33 + 72))( /*0x180001940*/
            a1: v33,
            a2: a1 + 88456,
            a3: 2,
            a4: 44100,
            a5: 0,
            a6: 2,
            a7: &v94,
            a8: &v96);
    if ( v21 >= 0 ) /*0x180001944*/
    {
      sub_180002C90(a1, a2: 0, a3: v36, a4: v37, a5: 1140457472); /*0x180001968*/
      v38 = a1 + 88480; /*0x18000196d*/
      goto LABEL_53; /*0x180001974*/
    }
LABEL_51:
    avs2_core_380(a1: "Class_XAudioControl::Init", a2: aXaudio2_8); /*0x180001adb*/
    goto LABEL_43; /*0x180001ae2*/
  }
  v39 = *(int **)(a1 + 88440); /*0x180001979*/
  v96 = 0; /*0x180001987*/
  v97 = v39; /*0x18000198c*/
  *(_DWORD *)&v128[8] = 1; /*0x1800019a5*/
  v95 = &v96; /*0x1800019ac*/
  v40 = *(_QWORD *)(a1 + 88576); /*0x1800019b1*/
  v129 = 2; /*0x1800019b8*/
  *(_QWORD *)v128 = v40; /*0x1800019c2*/
  v120 = 1; /*0x1800019d0*/
  v121 = v128; /*0x1800019d7*/
  v41 = *(__int64 (__fastcall **)(int, int, int, int, __int64))(a1 + 88584); /*0x1800019de*/
  v99 = 0x200000001LL; /*0x1800019e5*/
  v98 = v41; /*0x1800019ea*/
  *(_DWORD *)v100 = 1; /*0x1800019fc*/
  *(_QWORD *)&v100[4] = &v98; /*0x180001a01*/
  v21 = (*(__int64 (__fastcall **)(__int64 *, __int64, __int64, __int64, _DWORD, int, int *, int *))(*v33 + 72))( /*0x180001a26*/
          a1: v33,
          a2: a1 + 88464,
          a3: 2,
          a4: 44100,
          a5: 0,
          a6: 2,
          a7: &v94,
          a8: &v120);
  if ( v21 < 0 ) /*0x180001a2a*/
    goto LABEL_51; /*0x180001a2a*/
  sub_180002C90(a1, a2: 0, a3: v42, a4: v43, a5: 1140457472); /*0x180001a51*/
  v21 = (*(__int64 (__fastcall **)(__int64 *, __int64, __int64, __int64, _DWORD, int, int *, _QWORD))(**v8 + 72))( /*0x180001a8e*/
          a1: *v8,
          a2: a1 + 88488,
          a3: 2,
          a4: 44100,
          a5: 0,
          a6: 2,
          a7: &v94,
          a8: 0);
  if ( v21 < 0 ) /*0x180001a92*/
  {
LABEL_54:
    avs2_core_380(a1: "Class_XAudioControl::Init", a2: aXaudio2_9); /*0x180001b3c*/
    goto LABEL_43; /*0x180001b43*/
  }
  v21 = (*(__int64 (__fastcall **)(__int64 *, __int64, __int64, __int64, _DWORD, int, int *, _BYTE *))(**v8 + 72))( /*0x180001ad5*/
          a1: *v8,
          a2: a1 + 88472,
          a3: 2,
          a4: 44100,
          a5: 0,
          a6: 2,
          a7: &v94,
          a8: v100);
  if ( v21 < 0 ) /*0x180001ad9*/
    goto LABEL_51; /*0x180001ad9*/
  sub_180002C90(a1, a2: 1, a3: v44, a4: v45, a5: 1140457472); /*0x180001af9*/
  v38 = a1 + 88496; /*0x180001afe*/
LABEL_53:
  v21 = (*(__int64 (__fastcall **)(__int64 *, __int64, __int64, __int64, _DWORD, int, int *, _QWORD))(**v8 + 72))( /*0x180001b05*/
          a1: *v8,
          a2: v38,
          a3: 2,
          a4: 44100,
          a5: 0,
          a6: 2,
          a7: &v94,
          a8: 0);
  if ( v21 < 0 ) /*0x180001b3a*/
    goto LABEL_54; /*0x180001b3a*/
  v46 = *v8; /*0x180001b48*/
  v99 = 0x200000001LL; /*0x180001b50*/
  *(_DWORD *)v100 = 1; /*0x180001b71*/
  if ( *(_DWORD *)(a1 + 88380) == 0 ) /*0x180001b8e*/
  {
    v47 = *(_QWORD *)(a1 + 88480); /*0x180001b94*/
    *(_DWORD *)v128 = 0; /*0x180001b9b*/
    v129 = 0; /*0x180001ba2*/
    *(_QWORD *)&v128[4] = v47; /*0x180001ba9*/
    v48 = *(_QWORD *)(a1 + 88456); /*0x180001bb0*/
    v94 = 2; /*0x180001bb7*/
    v130 = v48; /*0x180001bbc*/
    v95 = (int *)v128; /*0x180001bca*/
    v49 = *v11; /*0x180001bcf*/
    v50 = (_QWORD *)(a1 + 88512); /*0x180001bd2*/
    v98 = v49; /*0x180001bd9*/
    *(_QWORD *)&v100[4] = &v98; /*0x180001be6*/
    v21 = (*(__int64 (__fastcall **)(__int64 *, __int64, __int64, __int64, _DWORD, int, int *, _BYTE *))(*v46 + 72))( /*0x180001bf1*/
            a1: v46,
            a2: a1 + 88512,
            a3: 2,
            a4: 44100,
            a5: 0,
            a6: 1,
            a7: &v94,
            a8: v100);
    if ( v21 >= 0 ) /*0x180001bf5*/
    {
      v51 = (_QWORD *)(a1 + 88520); /*0x180001c10*/
      v21 = (*(__int64 (__fastcall **)(__int64 *, __int64, __int64, __int64, _DWORD, int, int *, _QWORD))(**v8 + 72))( /*0x180001c33*/
              a1: *v8,
              a2: a1 + 88520,
              a3: 2,
              a4: 44100,
              a5: 0,
              a6: 1,
              a7: &v94,
              a8: 0);
      if ( v21 >= 0 ) /*0x180001c37*/
      {
        sub_180002AC0(a1: &unk_180029254, a2: a1); /*0x180001c47*/
        (*(void (__fastcall **)(_QWORD, _QWORD, __int64, __int64, _DWORD))(*(_QWORD *)*v50 + 48LL))( /*0x180001c62*/
          a1: *v50,
          a2: 0,
          a3: a1,
          a4: 52,
          a5: 0);
        v121 = (_BYTE *)*v50; /*0x180001c68*/
        v52 = *v51; /*0x180001c6f*/
        v120 = 0; /*0x180001c72*/
        v123 = v52; /*0x180001c79*/
        v53 = &v120; /*0x180001c80*/
        v122 = 0; /*0x180001c87*/
        goto LABEL_63; /*0x180001c8e*/
      }
LABEL_61:
      avs2_core_380(a1: "Class_XAudioControl::Init", a2: aXaudio2_10); /*0x180001d65*/
      goto LABEL_43; /*0x180001d6c*/
    }
LABEL_64:
    avs2_core_380(a1: "Class_XAudioControl::Init", a2: aXaudio2_11); /*0x180001e10*/
    goto LABEL_43; /*0x180001e17*/
  }
  v54 = *(_BYTE **)(a1 + 88488); /*0x180001c93*/
  v120 = 0; /*0x180001c9a*/
  v122 = 0; /*0x180001ca1*/
  v121 = v54; /*0x180001ca8*/
  v55 = *(_QWORD *)(a1 + 88496); /*0x180001caf*/
  v124 = 0; /*0x180001cb6*/
  v123 = v55; /*0x180001cbd*/
  v56 = *(_QWORD *)(a1 + 88464); /*0x180001cc4*/
  v126 = 0; /*0x180001ccb*/
  v125 = v56; /*0x180001cd2*/
  v57 = *(_QWORD *)(a1 + 88472); /*0x180001cd9*/
  v94 = 4; /*0x180001ce0*/
  v127 = v57; /*0x180001ce8*/
  v95 = &v120; /*0x180001cf6*/
  v58 = *v11; /*0x180001cfb*/
  v50 = (_QWORD *)(a1 + 88512); /*0x180001cfe*/
  v98 = v58; /*0x180001d05*/
  *(_QWORD *)&v100[4] = &v98; /*0x180001d12*/
  v21 = (*(__int64 (__fastcall **)(__int64 *, __int64, __int64, __int64, _DWORD, int, int *, _BYTE *))(*v46 + 72))( /*0x180001d1d*/
          a1: v46,
          a2: a1 + 88512,
          a3: 2,
          a4: 44100,
          a5: 0,
          a6: 1,
          a7: &v94,
          a8: v100);
  if ( v21 < 0 ) /*0x180001d21*/
    goto LABEL_64; /*0x180001d21*/
  v51 = (_QWORD *)(a1 + 88520); /*0x180001d3c*/
  v21 = (*(__int64 (__fastcall **)(__int64 *, __int64, __int64, __int64, _DWORD, int, int *, _QWORD))(**v8 + 72))( /*0x180001d5f*/
          a1: *v8,
          a2: a1 + 88520,
          a3: 2,
          a4: 44100,
          a5: 0,
          a6: 1,
          a7: &v94,
          a8: 0);
  if ( v21 < 0 ) /*0x180001d63*/
    goto LABEL_61; /*0x180001d63*/
  sub_180002AC0(a1: &unk_180029254, a2: a1); /*0x180001d7b*/
  (*(void (__fastcall **)(_QWORD, _QWORD, __int64, __int64, _DWORD))(*(_QWORD *)*v50 + 48LL))( /*0x180001d96*/
    a1: *v50,
    a2: 0,
    a3: a1,
    a4: 52,
    a5: 0);
  *(_QWORD *)&v128[4] = *v50; /*0x180001d9c*/
  v59 = *v51; /*0x180001da3*/
  *(_DWORD *)v128 = 0; /*0x180001da6*/
  v130 = v59; /*0x180001dad*/
  v53 = (int *)v128; /*0x180001db4*/
  v129 = 0; /*0x180001dbb*/
LABEL_63:
  v60 = *v8; /*0x180001dc2*/
  v96 = 2; /*0x180001dd4*/
  v97 = v53; /*0x180001ddc*/
  v61 = (__int64 **)(a1 + 88504); /*0x180001de4*/
  v21 = (*(__int64 (__fastcall **)(__int64 *, __int64, __int64, __int64, _DWORD, int, int *, _QWORD))(*v60 + 72))( /*0x180001e0a*/
          a1: v60,
          a2: a1 + 88504,
          a3: 2,
          a4: 44100,
          a5: 0,
          a6: 1,
          a7: &v96,
          a8: 0);
  if ( v21 < 0 ) /*0x180001e0e*/
    goto LABEL_64; /*0x180001e0e*/
  v62 = *v61; /*0x180001e1c*/
  v63 = *v51; /*0x180001e20*/
  v64 = **v61; /*0x180001e2a*/
  if ( *(_DWORD *)(a1 + 88380) != 0 ) /*0x180001e47*/
  {
    v66 = (*(__int64 (__fastcall **)(__int64 *, __int64, __int64))(v64 + 128))(a1: v62, a2: v63, a3: 2); /*0x180002051*/
    if ( v66 >= 0 ) /*0x180002056*/
    {
      v68 = (*(__int64 (__fastcall **)(__int64 *, _QWORD, __int64, __int64, void *, _DWORD))(**v61 + 128))( /*0x180002091*/
              a1: *v61,
              a2: *v50,
              a3: 2,
              a4: 2,
              a5: &unk_180029040,
              a6: 0);
      if ( v68 >= 0 ) /*0x180002096*/
      {
        v68 = (*(__int64 (__fastcall **)(_QWORD, _QWORD, __int64, __int64, void *, _DWORD))(*(_QWORD *)*v50 + 128LL))( /*0x1800020c2*/
                a1: *v50,
                a2: *(_QWORD *)(a1 + 88488),
                a3: 2,
                a4: 2,
                a5: &unk_180029040,
                a6: 0);
        if ( v68 >= 0 ) /*0x1800020c7*/
        {
          v68 = (*(__int64 (__fastcall **)(_QWORD, _QWORD, __int64, __int64, void *, _DWORD))(*(_QWORD *)*v50 + 128LL))( /*0x1800020ef*/
                  a1: *v50,
                  a2: *(_QWORD *)(a1 + 88496),
                  a3: 2,
                  a4: 2,
                  a5: &unk_180029040,
                  a6: 0);
          if ( v68 >= 0 ) /*0x1800020f4*/
          {
            v68 = (*(__int64 (__fastcall **)(_QWORD, _QWORD, __int64, __int64, void *, _DWORD))(*(_QWORD *)*v50 + 128LL))( /*0x18000211c*/
                    a1: *v50,
                    a2: *(_QWORD *)(a1 + 88464),
                    a3: 2,
                    a4: 2,
                    a5: &unk_180029040,
                    a6: 0);
            if ( v68 >= 0 ) /*0x180002121*/
            {
              v70 = (*(__int64 (__fastcall **)(_QWORD, _QWORD, __int64, __int64, void *, _DWORD))(*(_QWORD *)*v50 + 128LL))( /*0x180002159*/
                      a1: *v50,
                      a2: *(_QWORD *)(a1 + 88472),
                      a3: 2,
                      a4: 2,
                      a5: &unk_180029040,
                      a6: 0);
              if ( v70 >= 0 ) /*0x18000215e*/
              {
                v70 = (*(__int64 (__fastcall **)(_QWORD, _QWORD, __int64, __int64, void *, _DWORD))(*(_QWORD *)*v51 /*0x180002183*/
                                                                                                  + 128LL))(
                        a1: *v51,
                        a2: *(_QWORD *)(a1 + 88488),
                        a3: 2,
                        a4: 2,
                        a5: &unk_180029040,
                        a6: 0);
                if ( v70 >= 0 ) /*0x180002188*/
                {
                  v70 = (*(__int64 (__fastcall **)(_QWORD, _QWORD, __int64, __int64, void *, _DWORD))(*(_QWORD *)*v51 + 128LL))( /*0x1800021ad*/
                          a1: *v51,
                          a2: *(_QWORD *)(a1 + 88496),
                          a3: 2,
                          a4: 2,
                          a5: &unk_180029040,
                          a6: 0);
                  if ( v70 >= 0 ) /*0x1800021b2*/
                  {
                    v70 = (*(__int64 (__fastcall **)(_QWORD, _QWORD, __int64, __int64, void *, _DWORD))(*(_QWORD *)*v51 + 128LL))( /*0x1800021d7*/
                            a1: *v51,
                            a2: *(_QWORD *)(a1 + 88464),
                            a3: 2,
                            a4: 2,
                            a5: &unk_180029040,
                            a6: 0);
                    if ( v70 >= 0 ) /*0x1800021dc*/
                    {
                      v72 = (*(__int64 (__fastcall **)(_QWORD, _QWORD, __int64, __int64, void *, _DWORD))(*(_QWORD *)*v51 + 128LL))( /*0x180002211*/
                              a1: *v51,
                              a2: *(_QWORD *)(a1 + 88472),
                              a3: 2,
                              a4: 2,
                              a5: &unk_180029040,
                              a6: 0);
                      if ( v72 >= 0 ) /*0x180002215*/
                      {
                        v73 = (_QWORD *)(a1 + 88408); /*0x180002222*/
                        v72 = (*(__int64 (__fastcall **)(_QWORD, _QWORD, __int64, __int64, void *, _DWORD))(**(_QWORD **)(a1 + 88528) + 128LL))( /*0x180002245*/
                                a1: *(_QWORD *)(a1 + 88528),
                                a2: *(_QWORD *)(a1 + 88408),
                                a3: 2,
                                a4: 2,
                                a5: &unk_180029040,
                                a6: 0);
                        if ( v72 >= 0 ) /*0x180002249*/
                        {
                          v72 = (*(__int64 (__fastcall **)(_QWORD, _QWORD, __int64, __int64, void *, _DWORD))(**(_QWORD **)(a1 + 88424) + 128LL))( /*0x180002272*/
                                  a1: *(_QWORD *)(a1 + 88424),
                                  a2: *v73,
                                  a3: 2,
                                  a4: 2,
                                  a5: &unk_180029040,
                                  a6: 0);
                          if ( v72 >= 0 ) /*0x180002276*/
                          {
                            v72 = (*(__int64 (__fastcall **)(_QWORD, _QWORD, __int64, __int64, void *, _DWORD))(**(_QWORD **)(a1 + 88488) + 128LL))( /*0x1800022aa*/
                                    a1: *(_QWORD *)(a1 + 88488),
                                    a2: *(_QWORD *)(a1 + 88440),
                                    a3: 2,
                                    a4: 2,
                                    a5: &unk_180029050,
                                    a6: 0);
                            if ( v72 >= 0 ) /*0x1800022ae*/
                            {
                              v72 = (*(__int64 (__fastcall **)(_QWORD, _QWORD, __int64, __int64, void *, _DWORD))(**(_QWORD **)(a1 + 88496) + 128LL))( /*0x1800022e2*/
                                      a1: *(_QWORD *)(a1 + 88496),
                                      a2: *(_QWORD *)(a1 + 88440),
                                      a3: 2,
                                      a4: 2,
                                      a5: &unk_180029060,
                                      a6: 0);
                              if ( v72 >= 0 ) /*0x1800022e6*/
                              {
                                v72 = (*(__int64 (__fastcall **)(_QWORD, _QWORD, __int64, __int64, void *, _DWORD))(**(_QWORD **)(a1 + 88464) + 128LL))( /*0x18000231a*/
                                        a1: *(_QWORD *)(a1 + 88464),
                                        a2: *(_QWORD *)(a1 + 88440),
                                        a3: 2,
                                        a4: 2,
                                        a5: &unk_180029050,
                                        a6: 0);
                                if ( v72 >= 0 ) /*0x18000231e*/
                                {
                                  v72 = (*(__int64 (__fastcall **)(_QWORD, _QWORD, __int64, __int64, void *, _DWORD))(**(_QWORD **)(a1 + 88472) + 128LL))( /*0x18000234e*/
                                          a1: *(_QWORD *)(a1 + 88472),
                                          a2: *(_QWORD *)(a1 + 88440),
                                          a3: 2,
                                          a4: 2,
                                          a5: &unk_180029060,
                                          a6: 0);
                                  if ( v72 >= 0 ) /*0x180002352*/
                                  {
                                    v72 = (*(__int64 (__fastcall **)(_QWORD, _QWORD, __int64, __int64, void *, _DWORD))(**(_QWORD **)(a1 + 88440) + 128LL))( /*0x18000237b*/
                                            a1: *(_QWORD *)(a1 + 88440),
                                            a2: *(_QWORD *)(a1 + 88448),
                                            a3: 2,
                                            a4: 2,
                                            a5: &unk_180029040,
                                            a6: 0);
                                    if ( v72 >= 0 ) /*0x18000237f*/
                                    {
                                      v21 = (*(__int64 (__fastcall **)(_QWORD, _QWORD, __int64, __int64, void *, _DWORD))(**(_QWORD **)(a1 + 88448) + 128LL))( /*0x1800023b0*/
                                              a1: *(_QWORD *)(a1 + 88448),
                                              a2: *v73,
                                              a3: 2,
                                              a4: 2,
                                              a5: &unk_180029040,
                                              a6: 0);
                                      if ( v21 < 0 ) /*0x1800023b4*/
                                        goto LABEL_77; /*0x1800023b4*/
                                      goto LABEL_100; /*0x1800023b4*/
                                    }
                                  }
                                }
                              }
                            }
                          }
                        }
                        goto LABEL_98; /*0x18000237f*/
                      }
LABEL_114:
                      sub_1800027D0(a1, a2: v71); /*0x180002518*/
                      return (unsigned int)v72; /*0x180002522*/
                    }
                  }
                }
              }
LABEL_89:
              sub_1800027D0(a1, a2: v69); /*0x1800021de*/
              return (unsigned int)v70; /*0x1800021e9*/
            }
          }
        }
      }
LABEL_84:
      sub_1800027D0(a1, a2: v67); /*0x180002123*/
      return (unsigned int)v68; /*0x18000212e*/
    }
LABEL_79:
    sub_1800027D0(a1, a2: v65); /*0x180002058*/
    return (unsigned int)v66; /*0x180002063*/
  }
  v66 = (*(__int64 (__fastcall **)(__int64 *, __int64, __int64))(v64 + 128))(a1: v62, a2: v63, a3: 2); /*0x180001e53*/
  if ( v66 < 0 ) /*0x180001e58*/
    goto LABEL_79; /*0x180001e58*/
  v68 = (*(__int64 (__fastcall **)(__int64 *, _QWORD, __int64, __int64, void *, _DWORD))(**v61 + 128))( /*0x180001e87*/
          a1: *v61,
          a2: *v50,
          a3: 2,
          a4: 2,
          a5: &unk_180029040,
          a6: 0);
  if ( v68 < 0 ) /*0x180001e8c*/
    goto LABEL_84; /*0x180001e8c*/
  v68 = (*(__int64 (__fastcall **)(_QWORD, _QWORD, __int64, __int64, void *, _DWORD))(*(_QWORD *)*v50 + 128LL))( /*0x180001eb8*/
          a1: *v50,
          a2: *(_QWORD *)(a1 + 88480),
          a3: 2,
          a4: 2,
          a5: &unk_180029040,
          a6: 0);
  if ( v68 < 0 ) /*0x180001ebd*/
    goto LABEL_84; /*0x180001ebd*/
  v70 = (*(__int64 (__fastcall **)(_QWORD, _QWORD, __int64, __int64, void *, _DWORD))(*(_QWORD *)*v50 + 128LL))( /*0x180001ee9*/
          a1: *v50,
          a2: *(_QWORD *)(a1 + 88456),
          a3: 2,
          a4: 2,
          a5: &unk_180029040,
          a6: 0);
  if ( v70 < 0 ) /*0x180001eee*/
    goto LABEL_89; /*0x180001eee*/
  v70 = (*(__int64 (__fastcall **)(_QWORD, _QWORD, __int64, __int64, void *, _DWORD))(*(_QWORD *)*v51 + 128LL))( /*0x180001f17*/
          a1: *v51,
          a2: *(_QWORD *)(a1 + 88480),
          a3: 2,
          a4: 2,
          a5: &unk_180029040,
          a6: 0);
  if ( v70 < 0 ) /*0x180001f1c*/
    goto LABEL_89; /*0x180001f1c*/
  v72 = (*(__int64 (__fastcall **)(_QWORD, _QWORD, __int64, __int64, void *, _DWORD))(*(_QWORD *)*v51 + 128LL))( /*0x180001f45*/
          a1: *v51,
          a2: *(_QWORD *)(a1 + 88456),
          a3: 2,
          a4: 2,
          a5: &unk_180029040,
          a6: 0);
  if ( v72 < 0 ) /*0x180001f49*/
    goto LABEL_114; /*0x180001f49*/
  v73 = (_QWORD *)(a1 + 88408); /*0x180001f56*/
  v72 = (*(__int64 (__fastcall **)(_QWORD, _QWORD, __int64, __int64, void *, _DWORD))(**(_QWORD **)(a1 + 88528) + 128LL))( /*0x180001f79*/
          a1: *(_QWORD *)(a1 + 88528),
          a2: *(_QWORD *)(a1 + 88408),
          a3: 2,
          a4: 2,
          a5: &unk_180029040,
          a6: 0);
  if ( v72 < 0 /*0x18000200c*/
    || (v72 = (*(__int64 (__fastcall **)(_QWORD, _QWORD, __int64, __int64, void *, _DWORD))(**(_QWORD **)(a1 + 88424)
                                                                                          + 128LL))(
                a1: *(_QWORD *)(a1 + 88424),
                a2: *v73,
                a3: 2,
                a4: 2,
                a5: &unk_180029040,
                a6: 0)) < 0
    || (v72 = (*(__int64 (__fastcall **)(_QWORD, _QWORD, __int64, __int64, void *, _DWORD))(**(_QWORD **)(a1 + 88480)
                                                                                          + 128LL))(
                a1: *(_QWORD *)(a1 + 88480),
                a2: *(_QWORD *)(a1 + 88432),
                a3: 2,
                a4: 2,
                a5: &unk_180029040,
                a6: 0)) < 0
    || (v72 = (*(__int64 (__fastcall **)(_QWORD, _QWORD, __int64, __int64, void *, _DWORD))(**(_QWORD **)(a1 + 88456)
                                                                                          + 128LL))(
                a1: *(_QWORD *)(a1 + 88456),
                a2: *(_QWORD *)(a1 + 88432),
                a3: 2,
                a4: 2,
                a5: &unk_180029040,
                a6: 0)) < 0 )
  {
LABEL_98:
    avs2_core_380(a1: "Class_XAudioControl::Init", a2: aXaudio2_12); /*0x180002381*/
    goto LABEL_114; /*0x180002388*/
  }
  v21 = (*(__int64 (__fastcall **)(_QWORD, _QWORD, __int64, __int64, void *, _DWORD))(**(_QWORD **)(a1 + 88432) + 128LL))( /*0x180002035*/
          a1: *(_QWORD *)(a1 + 88432),
          a2: *v73,
          a3: 2,
          a4: 2,
          a5: &unk_180029040,
          a6: 0);
  if ( v21 < 0 ) /*0x180002039*/
  {
LABEL_77:
    avs2_core_380(a1: "Class_XAudioControl::Init", a2: aXaudio2_12); /*0x18000203f*/
    goto LABEL_43; /*0x180002046*/
  }
LABEL_100:
  if ( v119 == 2 ) /*0x1800023d2*/
  {
    v74 = (*(__int64 (__fastcall **)(_QWORD, _QWORD, __int64, __int64, void *, _DWORD))(*(_QWORD *)*v73 + 128LL))( /*0x180002405*/
            a1: *v73,
            a2: *(_QWORD *)(a1 + 88400),
            a3: 2,
            a4: 2,
            a5: &unk_180029000,
            a6: 0);
  }
  else
  {
    if ( v119 != 6 ) /*0x1800023d7*/
      goto LABEL_105; /*0x1800023d7*/
    v74 = (*(__int64 (__fastcall **)(_QWORD, _QWORD, __int64, __int64, void *, _DWORD))(*(_QWORD *)*v73 + 128LL))( /*0x1800023e6*/
            a1: *v73,
            a2: *(_QWORD *)(a1 + 88400),
            a3: 2,
            a4: 6,
            a5: &unk_180029010,
            a6: 0);
  }
  v21 = v74; /*0x180002412*/
LABEL_105:
  if ( v21 < 0 ) /*0x180002416*/
    goto LABEL_77; /*0x180002416*/
  if ( v119 == 2 ) /*0x180002426*/
  {
    v75 = (*(__int64 (__fastcall **)(_QWORD, _QWORD, __int64, __int64, void *, _DWORD))(**(_QWORD **)(a1 + 88416) + 128LL))( /*0x18000245d*/
            a1: *(_QWORD *)(a1 + 88416),
            a2: *(_QWORD *)(a1 + 88400),
            a3: 2,
            a4: 2,
            a5: &unk_180029000,
            a6: 0);
  }
  else
  {
    if ( v119 != 6 ) /*0x18000242b*/
      goto LABEL_111; /*0x18000242b*/
    v75 = (*(__int64 (__fastcall **)(_QWORD, _QWORD, __int64, __int64, void *, _DWORD))(**(_QWORD **)(a1 + 88416) + 128LL))( /*0x18000243a*/
            a1: *(_QWORD *)(a1 + 88416),
            a2: *(_QWORD *)(a1 + 88400),
            a3: 2,
            a4: 6,
            a5: &unk_180029010,
            a6: 0);
  }
  v21 = v75; /*0x180002463*/
LABEL_111:
  if ( v21 < 0 ) /*0x180002467*/
    goto LABEL_77; /*0x180002467*/
  avs2_core_244(a1: a1 + 88324, a2: 0, a3: 44); /*0x18000247a*/
  v76 = *v8; /*0x180002480*/
  *(_DWORD *)(a1 + 88328) = 88200; /*0x180002494*/
  *(_DWORD *)(a1 + 88356) = 255; /*0x18000249e*/
  *(_DWORD *)(a1 + 88324) = 64; /*0x1800024a8*/
  v116[0] = 65537; /*0x1800024b2*/
  v116[1] = 44100; /*0x1800024b9*/
  *(_QWORD *)(a1 + 88332) = a1 + 124; /*0x1800024c0*/
  v116[2] = 88200; /*0x1800024c7*/
  v116[3] = 1048578; /*0x1800024ce*/
  v117 = 0; /*0x1800024d5*/
  v72 = (*(__int64 (__fastcall **)(__int64 *, __int64, _DWORD *, _QWORD, int, _QWORD, _QWORD, _QWORD))(*v76 + 64))( /*0x1800024fe*/
          a1: v76,
          a2: a1 + 88368,
          a3: v116,
          a4: 0,
          a5: 0x40000000,
          a6: 0,
          a7: 0,
          a8: 0);
  if ( v72 < 0 ) /*0x180002502*/
  {
    avs2_core_380(a1: "Class_XAudioControl::Init", a2: aXaudio2_13); /*0x180002512*/
    goto LABEL_114; /*0x180002512*/
  }
  (*(void (__fastcall **)(_QWORD, __int64, _QWORD))(**(_QWORD **)(a1 + 88368) + 168LL))( /*0x18000253b*/
    a1: *(_QWORD *)(a1 + 88368),
    a2: a1 + 88324,
    a3: 0);
  (*(void (__fastcall **)(_QWORD, _QWORD, _QWORD))(**(_QWORD **)(a1 + 88368) + 152LL))( /*0x180002550*/
    a1: *(_QWORD *)(a1 + 88368),
    a2: 0,
    a3: 0);
  v77 = (__int64 **)(a1 + 88384); /*0x180002556*/
  v21 = sub_1800035C0(a1: 0, a2: a1 + 88384); /*0x180002567*/
  if ( v21 < 0 ) /*0x18000256b*/
  {
    avs2_core_380(a1: "Class_XAudioControl::Init", a2: aXact3createeng); /*0x18000257b*/
    goto LABEL_43; /*0x18000257b*/
  }
  memset(Buffer, 0, 260); /*0x18000259f*/
  GetCurrentDirectoryA(nBufferLength: 0x104u, lpBuffer: Buffer); /*0x1800025b7*/
  v78 = avs2_core_79(a1: a2, a2: 1, a3: 511); /*0x1800025d4*/
  *(_DWORD *)(a1 + 88600) = v78; /*0x1800025da*/
  if ( v78 > 0 ) /*0x1800025e2*/
  {
    v80 = sub_180003840(a1: (unsigned int)v78); /*0x18000260f*/
    *(_DWORD *)(a1 + 88604) = v80; /*0x180002614*/
    if ( v80 == -1 ) /*0x18000261d*/
    {
      sub_1800027D0(a1, a2: v81); /*0x180002622*/
      return 2147500037LL; /*0x180002627*/
    }
    else
    {
      v82 = CoTaskMemAlloc(cb: v80); /*0x180002633*/
      *(_QWORD *)(a1 + 88608) = v82; /*0x180002639*/
      if ( v82 != nullptr ) /*0x180002643*/
      {
        if ( (int)avs2_core_82(a1: *(unsigned int *)(a1 + 88600), a2: v82, a3: *(unsigned int *)(a1 + 88604)) >= 0 ) /*0x18000266f*/
        {
          v85 = *(unsigned int *)(a1 + 88600); /*0x180002683*/
          if ( (int)v85 > 0 ) /*0x18000268b*/
          {
            avs2_core_86(a1: v85); /*0x18000268d*/
            *(_DWORD *)(a1 + 88600) = 0; /*0x180002693*/
          }
          v86 = *v77; /*0x18000269a*/
          v110 = 1; /*0x1800026a4*/
          v99 = (__int64)DllMain; /*0x1800026a8*/
          v113 = 0; /*0x1800026b4*/
          v112 = DllMain; /*0x1800026be*/
          v98 = sub_180003880; /*0x1800026c3*/
          v87 = *(_QWORD *)(a1 + 88608); /*0x1800026c8*/
          v107 = 250; /*0x1800026cf*/
          v111 = sub_180003880; /*0x1800026dc*/
          v108 = v87; /*0x1800026e1*/
          v109 = *(_DWORD *)(a1 + 88604); /*0x1800026ef*/
          v114 = *v8; /*0x1800026f5*/
          v115 = *(_QWORD *)(a1 + 88400); /*0x180002700*/
          v21 = (*(__int64 (__fastcall **)(__int64 *, int *))(*v86 + 48))(a1: v86, a2: &v107); /*0x18000270a*/
          if ( v21 < 0 ) /*0x18000270e*/
            goto LABEL_43; /*0x18000270e*/
          v88 = &unk_180038838; /*0x180002714*/
          for ( i = 1024; i != 0; --i ) /*0x18000271b*/
          {
            *(v88 - 1) = 0; /*0x180002725*/
            *v88 = 0; /*0x180002729*/
            v88[1] = 0; /*0x18000272c*/
            v88[2] = 0; /*0x180002730*/
            v88[3] = 0; /*0x180002734*/
            v88[4] = 0; /*0x180002738*/
            v88[5] = 0; /*0x18000273c*/
            sub_180005A30(a1: v88); /*0x180002740*/
            v88 += 7; /*0x180002745*/
          }
          v90 = *v77; /*0x18000274e*/
          if ( *v77 != nullptr ) /*0x180002754*/
          {
            v91 = *v90; /*0x180002756*/
            (*(void (__fastcall **)(__int64 *, const char *))(*v90 + 136))(a1: v90, a2: "Global"); /*0x180002760*/
            (*(void (__fastcall **)(__int64 *, _QWORD))(v91 + 152))(a1: *v77, a2: v92); /*0x180002774*/
          }
          return 0; /*0x18000277a*/
        }
        else
        {
          sub_1800027D0(a1, a2: v84); /*0x180002674*/
          return 2147549183LL; /*0x180002679*/
        }
      }
      else
      {
        sub_1800027D0(a1, a2: v83); /*0x180002648*/
        return 2147942414LL; /*0x18000264d*/
      }
    }
  }
  else
  {
    avs2_core_380(a1: "Class_XAudioControl::Init", a2: aXact3, a2); /*0x1800025f5*/
    sub_1800027D0(a1, a2: v79); /*0x1800025fe*/
    return 2147942406LL; /*0x180002603*/
  }
}
