void __fastcall CLibXact::execute(CLibXact *this)
{
  CLibXact *v1; // rdi
  char *v2; // rbx
  char *v3; // rsi
  int v4; // r15d
  int v5; // r8d
  int v6; // r9d
  int v7; // eax
  unsigned __int64 v8; // rdx
  int v9; // edx
  CLibXact *v10; // rcx
  unsigned int v11; // r13d
  __int64 v12; // r12
  int v13; // r14d
  int v14; // edi
  __int64 v15; // rsi
  int *v16; // rbx
  char *v17; // rbx
  __int64 i; // rdi
  __int64 v19; // rbx
  _DWORD *v20; // rax
  __int64 v21; // rbx
  __int64 v22; // r12
  unsigned int v23; // r12d
  int v24; // r14d
  int v25; // edi
  __int64 v26; // rsi
  int *v27; // rbx
  char *v28; // rbx
  __int64 j; // rdi
  __int64 v30; // rdx
  unsigned int v31; // ebx
  _DWORD *v32; // rax
  __int64 v33; // rdi
  __int64 v34; // rbx
  __int64 v35; // rbx
  __int64 v36; // rax
  __int64 v37; // rbx
  int v38; // ebx
  int v39; // eax
  unsigned int v40; // ebx
  __int64 v41; // rax
  int v42; // xmm8_4
  int v43; // xmm9_4
  HWAVEIN v44; // rax
  int v45; // eax
  int v46; // xmm8_4
  int v47; // xmm9_4
  int v48; // eax
  int v49; // eax
  int *v50; // rdi
  char *v51; // r14
  __int64 k; // r15
  __int64 v53; // rax
  int v54; // esi
  int v55; // ebx
  __int64 v56; // rax
  __int64 v57; // rax
  __int64 v58; // r13
  int v59; // esi
  int v60; // ebx
  unsigned __int16 v61; // r14
  _DWORD *v62; // rdi
  int v63; // ebx
  __int64 v64; // r12
  __int64 v65; // rax
  int v66; // eax
  int v67; // r8d
  int v68; // r9d
  int v69; // ebx
  __int64 v70; // rdi
  _DWORD *v71; // rax
  int v72; // r14d
  __int64 v73; // rax
  int v74; // ebx
  __int64 v75; // rdi
  _DWORD *v76; // rax
  int v77; // r15d
  __int64 v78; // rax
  __int64 v79; // rdi
  __int64 v80; // rbx
  __int64 v81; // r13
  __int64 v82; // r12
  char *v83; // r14
  int v84; // edx
  __int64 v85; // rcx
  _DWORD *v86; // rax
  __int64 v87; // rcx
  __int64 v88; // rbx
  __int64 v89; // rax
  int v90; // r8d
  int v91; // r9d
  HWAVEIN v92; // rax
  HWAVEIN v93; // rax
  HWAVEIN v94; // rax
  unsigned __int16 v95; // ax
  unsigned __int16 *v96; // rax
  unsigned __int16 *v97; // rax
  unsigned __int16 *v98; // rax
  _DWORD *v99; // r8
  _DWORD *v100; // rcx
  int v101; // esi
  unsigned int v102; // r13d
  __int64 v103; // r12
  int v104; // ebx
  __int64 v105; // rdi
  __int64 v106; // rdi
  __int64 v107; // rax
  int v108; // r14d
  int v109; // ebx
  int v110; // r8d
  int v112; // [rsp+40h] [rbp-C8h]
  HWAVEIN phwi[73]; // [rsp+60h] [rbp-A8h] BYREF
  _BYTE v114[8]; // [rsp+2A8h] [rbp+1A0h] BYREF
  WAVEFORMATEX pwfx; // [rsp+2B0h] [rbp+1A8h] BYREF
  _QWORD v116[6]; // [rsp+2C8h] [rbp+1C0h] BYREF
  char Buffer[272]; // [rsp+2F8h] [rbp+1F0h] BYREF
  char v118[272]; // [rsp+408h] [rbp+300h] BYREF

  v1 = this; /*0x180009332*/
  CoInitializeEx(pvReserved: nullptr, dwCoInit: 0); /*0x18000933e*/
  *((_DWORD *)v1 + 145415) = 0; /*0x180009344*/
  __eh34_enter_wind_state(-1, 0); /*0xf1c000000000004c*/
  while ( 1 ) /*0x180009362*/
  {
    v2 = (char *)v1 + 308; /*0x180009362*/
    v3 = (char *)v1 + 48; /*0x180009369*/
    while ( 2 ) /*0x180009370*/
    {
      v4 = -2147467259; /*0x180009370*/
      avs2_core_16(a1: *((unsigned int *)v1 + 145413)); /*0x18000937c*/
      memset(a1: &phwi[1], Val: 0, Size: 0x238u); /*0x18000938f*/
      LOBYTE(phwi[1]) = *((_BYTE *)v1 + 8); /*0x180009398*/
      HIDWORD(phwi[1]) = *((_DWORD *)v1 + 3); /*0x18000939f*/
      phwi[2] = *((HWAVEIN *)v1 + 2); /*0x1800093a6*/
      phwi[3] = *((HWAVEIN *)v1 + 3); /*0x1800093b6*/
      phwi[4] = *((HWAVEIN *)v1 + 4); /*0x1800093cc*/
      phwi[5] = *((HWAVEIN *)v1 + 5); /*0x1800093e1*/
      if ( v3 != nullptr ) /*0x1800093e8*/
        avs2_core_223(a1: &phwi[6], a2: v3, a3: 260); /*0x1800093f7*/
      if ( v2 != nullptr ) /*0x180009400*/
        avs2_core_223(a1: (char *)&phwi[38] + 4, a2: v2, a3: 260); /*0x180009412*/
      avs2_core_17(a1: *((unsigned int *)v1 + 145413)); /*0x18000941e*/
      v7 = HIDWORD(phwi[1]); /*0x180009424*/
      if ( HIDWORD(phwi[1]) == 0 ) /*0x18000942a*/
      {
        Sleep(dwMilliseconds: 2u); /*0x18000942f*/
        v7 = HIDWORD(phwi[1]); /*0x180009435*/
      }
      v8 = 0x180000000uLL; /*0x180009446*/
      switch ( v7 ) /*0x180009457*/
      {
        case 1: /*0x180009457*/
          if ( __eh34_unwind(0) ) /*0xf1c0000000000040*/
            goto unwind_state_0; /*0xf1c0000000000040*/
          __eh34_exit_wind_state(0, -1); /*0xf1c0000000000044*/
          *((_DWORD *)v1 + 145415) = 1; /*0x180009459*/
          phwi[0] = nullptr; /*0x180009460*/
          pwfx.cbSize = 0; /*0x180009472*/
          *(_DWORD *)&pwfx.wFormatTag = 65537; /*0x180009479*/
          pwfx.nSamplesPerSec = 22050; /*0x180009483*/
          *(_DWORD *)&pwfx.nBlockAlign = 524289; /*0x18000948d*/
          pwfx.nAvgBytesPerSec = 22050; /*0x180009497*/
          waveInOpen(phwi, uDeviceID: 0xFFFFFFFF, &pwfx, dwCallback: 0, dwInstance: 0, fdwOpen: 0); /*0x1800094bc*/
          if ( phwi[0] != nullptr ) /*0x1800094ca*/
            waveInClose(hwi: phwi[0]); /*0x1800094cc*/
          __eh34_enter_wind_state(-1, 0); /*0xf1c0000000000038*/
          v9 = sub_1800012E0(a1: &unk_188F5FEC0, a2: &phwi[6], a3: LODWORD(phwi[2])); /*0x1800094e7*/
          *((_DWORD *)v1 + 145415) = (v9 < 0) + 2; /*0x1800094f3*/
          v10 = v1; /*0x1800094f9*/
          if ( LOBYTE(phwi[1]) == 0 ) /*0x180009501*/
            goto LABEL_13; /*0x180009501*/
          goto LABEL_192; /*0x180009501*/
        case 2: /*0x180009457*/
          *((_DWORD *)v1 + 145415) = 0; /*0x18000a869*/
          sub_1800027D0(a1: &unk_188F5FEC0, a2: 0x180000000uLL); /*0x18000a87a*/
          if ( LOBYTE(phwi[1]) != 0 ) /*0x18000a887*/
            CLibXact::FinishCommand(this: v1, a2: 0); /*0x18000a88b*/
          else
            CLibXact::del_cue(this: v1); /*0x18000a892*/
          CoUninitialize(); /*0x18000a897*/
          avs2_core_11(a1: 0); /*0x18000a89f*/
          if ( __eh34_unwind(0) ) /*0xf1c0000000000010*/
            goto unwind_state_0; /*0xf1c0000000000010*/
          __eh34_exit_wind_state(0, -1); /*0xf1c000000000001c*/
          return; /*0x18000a8a5*/
        case 3: /*0x180009457*/
          v9 = dword_188F758E8; /*0x180009a62*/
          v10 = v1; /*0x180009a68*/
          goto LABEL_192; /*0x180009a6b*/
        case 4: /*0x180009457*/
          memset(v116, 0, sizeof(v116)); /*0x180009a70*/
          v116[0] = *(_QWORD *)SLODWORD(phwi[2]); /*0x180009a94*/
          v116[1] = *(_QWORD *)(SLODWORD(phwi[2]) + 8LL); /*0x180009a9f*/
          v116[2] = *(_QWORD *)(SLODWORD(phwi[2]) + 0x10LL); /*0x180009aaa*/
          v116[3] = *(_QWORD *)(SLODWORD(phwi[2]) + 0x18LL); /*0x180009ab5*/
          v116[4] = *(_QWORD *)(SLODWORD(phwi[2]) + 0x20LL); /*0x180009ac0*/
          v116[5] = *(_QWORD *)(SLODWORD(phwi[2]) + 0x28LL); /*0x180009acb*/
          v38 = sub_180001000(a1: &unk_188F5FEC0, a2: v116, a3: 48); /*0x180009aeb*/
          goto LABEL_74; /*0x180009aeb*/
        case 5: /*0x180009457*/
          v39 = avs2_core_79(a1: &phwi[6], a2: 1, a3: 511); /*0x180009b1e*/
          v40 = v39; /*0x180009b24*/
          if ( v39 <= 0 ) /*0x180009b28*/
          {
            avs2_core_380(a1: "CLibXact::SetMasteringEq", a2: &unk_18001C2D0, a3: &phwi[6]); /*0x180009b3c*/
            goto LABEL_190; /*0x180009b42*/
          }
          if ( (unsigned int)sub_180003840(a1: (unsigned int)v39) != 48 ) /*0x180009b51*/
          {
            avs2_core_380(a1: "CLibXact::SetMasteringEq", a2: &unk_18001C310, a3: &phwi[6]); /*0x180009b65*/
LABEL_190:
            v10 = v1; /*0x18000a800*/
LABEL_191:
            v9 = 0; /*0x18000a803*/
            goto LABEL_192; /*0x18000a803*/
          }
          memset(v116, 0, sizeof(v116)); /*0x180009b70*/
          avs2_core_82(a1: v40, a2: v116, a3: 48); /*0x180009b9b*/
          avs2_core_86(a1: v40); /*0x180009ba3*/
          v38 = sub_180001000(a1: &unk_188F5FEC0, a2: v116, a3: 48); /*0x180009bc2*/
          if ( v38 != 0 ) /*0x180009bc6*/
          {
LABEL_74:
            (*(void (__fastcall **)(__int64, _QWORD, _QWORD))(*(_QWORD *)qword_188F75810 + 24LL))( /*0x180009aed*/
              a1: qword_188F75810,
              a2: 0,
              a3: 0);
            dword_188F758E8 = 1; /*0x180009b00*/
          }
LABEL_75:
          v9 = v38; /*0x180009b07*/
LABEL_76:
          v10 = v1; /*0x180009b09*/
LABEL_192:
          CLibXact::FinishCommand(this: v10, a2: v9); /*0x18000a805*/
LABEL_193:
          v2 = (char *)v1 + 308; /*0x18000a80a*/
          v3 = (char *)v1 + 48; /*0x18000a818*/
          if ( *((_DWORD *)v1 + 145415) != 2 ) /*0x18000a81c*/
            continue; /*0x18000a81c*/
          v110 = dword_189646968; /*0x18000a822*/
          if ( dword_189646968 == 10 * (dword_189646968 / 10) ) /*0x18000a843*/
          {
            (*(void (__fastcall **)(__int64))(*(_QWORD *)qword_188F75800 + 64LL))(a1: qword_188F75800); /*0x18000a84f*/
            sub_180007CD0(); /*0x18000a852*/
            v110 = 0; /*0x18000a857*/
          }
          dword_189646968 = v110 + 1; /*0x18000a85d*/
          break; /*0x18000a864*/
        case 6: /*0x180009457*/
          v41 = *(_QWORD *)qword_188F75810; /*0x180009bdd*/
          if ( LODWORD(phwi[2]) != 0 ) /*0x180009be4*/
          {
            (*(void (__fastcall **)(__int64, _QWORD, _QWORD))(v41 + 24))(a1: qword_188F75810, a2: 0, a3: 0); /*0x180009be6*/
            dword_188F758E8 = 1; /*0x180009be9*/
          }
          else
          {
            (*(void (__fastcall **)(__int64, _QWORD, _QWORD))(v41 + 32))(a1: qword_188F75810, a2: 0, a3: 0); /*0x180009bf5*/
            dword_188F758E8 = 0; /*0x180009bf8*/
          }
          goto LABEL_190; /*0x180009bf0*/
        case 7: /*0x180009457*/
          CLibXact::CLibXact(this: (CLibXact *)v114); /*0x180009c25*/
          sub_180006650(); /*0x180009c32*/
          CLibXact::del_cue(this: v1); /*0x180009c3a*/
          if ( __eh34_unwind(0) ) /*0xf1c000000000002c*/
          {
unwind_state_0:
            sub_180006530(a1: v114); /*0xf1c0000000000014*/
            __eh34_propagate_exception_into_caller(0, -1); /*0xf1c0000000000018*/
          }
          __eh34_exit_wind_state(0, -1); /*0xf1c0000000000030*/
          sub_180006530(a1: v114); /*0x180009c47*/
          __eh34_enter_wind_state(-1, 0); /*0xf1c0000000000024*/
          goto LABEL_193; /*0xf1c0000000000028*/
        case 8: /*0x180009457*/
          dword_180029930 = (int)phwi[2]; /*0x180009c55*/
          sub_180003160(a1: &unk_188F5FEC0, a2: 0x180000000uLL); /*0x180009c62*/
          CLibXact::del_cue(this: v1); /*0x180009c6a*/
          goto LABEL_193; /*0x180009c6f*/
        case 9: /*0x180009457*/
          dword_180029938 = (int)phwi[2]; /*0x180009c78*/
          sub_180003160(a1: &unk_188F5FEC0, a2: 0x180000000uLL); /*0x180009c85*/
          goto LABEL_190; /*0x180009c8a*/
        case 10: /*0x180009457*/
          dword_188F5FEB4 = (int)phwi[2]; /*0x180009ca8*/
          sub_180003160(a1: &unk_188F5FEC0, a2: 0x180000000uLL); /*0x180009cb5*/
          CLibXact::del_cue(this: v1); /*0x180009cbd*/
          goto LABEL_193; /*0x180009cc2*/
        case 11: /*0x180009457*/
          dword_180029934 = (int)phwi[2]; /*0x180009ccb*/
          sub_180003160(a1: &unk_188F5FEC0, a2: 0x180000000uLL); /*0x180009cd8*/
          CLibXact::del_cue(this: v1); /*0x180009ce0*/
          goto LABEL_193; /*0x180009ce5*/
        case 12: /*0x180009457*/
          sub_180002C80(a1: &unk_188F5FEC0, a2: LODWORD(phwi[2])); /*0x180009c9a*/
          goto LABEL_190; /*0x180009c9f*/
        case 13: /*0x180009457*/
          sub_180002C90(a1: (unsigned int)&unk_188F5FEC0, a2: phwi[2], a3: v5, a4: v6, a5: phwi[4]); /*0x180009d0d*/
          CLibXact::del_cue(this: v1); /*0x180009d15*/
          goto LABEL_193; /*0x180009d1a*/
        case 14: /*0x180009457*/
          if ( LODWORD(phwi[2]) <= 0x1D ) /*0x180009d26*/
          {
            dword_18002993C = (int)phwi[2]; /*0x180009d28*/
            sub_1800034C0(a1: &unk_188F5FEC0, a2: 0x180000000uLL); /*0x180009d35*/
          }
          if ( HIDWORD(phwi[2]) <= 0x7F ) /*0x180009d41*/
          {
            *(float *)&dword_180029968 = (float)SHIDWORD(phwi[2]); /*0x180009d4a*/
            sub_180008780(a1: &unk_188F5FEC0, a2: v8); /*0x180009d59*/
          }
          goto LABEL_107; /*0x180009d61*/
        case 15: /*0x180009457*/
          sub_180002F40(a1: (unsigned int)&unk_188F5FEC0, a2: phwi[2], a3: v5, a4: v6, a5: phwi[4]); /*0x180009d8e*/
          CLibXact::del_cue(this: v1); /*0x180009d96*/
          goto LABEL_193; /*0x180009d9b*/
        case 16: /*0x180009457*/
          v11 = 0; /*0x180009511*/
          v12 = 0; /*0x180009514*/
          v13 = 0; /*0x180009517*/
          if ( byte_180038820 != 0 ) /*0x180009521*/
          {
            avs2_core_382(a1: "CLibXact::LoadWaveBank", a2: &unk_18001C1C8); /*0x180009531*/
            v4 = 0; /*0x180009537*/
            goto LABEL_39; /*0x18000953a*/
          }
          v14 = 0; /*0x18000953f*/
          v15 = 0; /*0x180009541*/
          v16 = (int *)&unk_180046830; /*0x180009543*/
          do /*0x18000958c*/
          {
            if ( v16[1] >= 0 /*0x18000957b*/
              && *v16 >= 0
              && (unsigned int)avs2_core_229(a1: (char *)&unk_180046838 + 788 * v14, a2: &phwi[6]) == 0 )
            {
              avs2_core_244(a1: (char *)&unk_180046830 + 788 * v14, a2: 0, a3: 788); /*0x1800095ac*/
              *(_QWORD *)((char *)&unk_180046830 + 788 * v15) = -1; /*0x1800095b9*/
              avs2_core_382(a1: "CLibXact::LoadWaveBank", a2: &unk_18001C1C8); /*0x1800095cf*/
              byte_180038820 = 1; /*0x1800095d5*/
              v4 = 0; /*0x1800095dc*/
LABEL_38:
              v1 = this; /*0x180009710*/
              goto LABEL_39; /*0x180009710*/
            }
            ++v14; /*0x18000957d*/
            ++v15; /*0x18000957f*/
            v16 += 197; /*0x180009582*/
          }
          while ( v14 < 64 ); /*0x18000958c*/
          v17 = (char *)&unk_188F75928; /*0x1800095e4*/
          for ( i = 128; i != 0; --i ) /*0x1800095eb*/
          {
            if ( (unsigned int)avs2_core_229(a1: v17, a2: &phwi[6]) == 0 ) /*0x1800095ff*/
            {
              avs2_core_380(a1: "CLibXact::LoadWaveBank", a2: &unk_18001C218, a3: &phwi[6]); /*0x180009613*/
              v4 = -2147467259; /*0x180009619*/
              v13 = 1; /*0x18000961f*/
            }
            v17 += 704; /*0x180009625*/
          }
          if ( v13 != 0 ) /*0x180009634*/
            goto LABEL_38; /*0x180009634*/
          v19 = 0; /*0x18000963a*/
          v20 = &unk_188F75920; /*0x180009643*/
          while ( *v20 != -1 ) /*0x180009653*/
          {
            LODWORD(i) = i + 1; /*0x180009655*/
            ++v19; /*0x180009657*/
            v20 += 176; /*0x18000965a*/
            if ( (__int64)v20 >= (__int64)&unk_188F8B920 ) /*0x180009663*/
              goto LABEL_33; /*0x180009663*/
          }
          v11 = i; /*0x180009667*/
          v12 = v19; /*0x18000966a*/
          v21 = 704 * v19; /*0x18000966d*/
          *(_DWORD *)((char *)&unk_188F75920 + v21) = i; /*0x180009674*/
          avs2_core_223(a1: (char *)&unk_188F75928 + 704 * (int)i, a2: &phwi[6], a3: 260); /*0x180009695*/
          *(_DWORD *)((char *)&unk_188F75920 + v21 + 4) = 5; /*0x18000969b*/
LABEL_33:
          if ( (_DWORD)i == 128 ) /*0x1800096ad*/
          {
            avs2_core_380(a1: "CLibXact::LoadWaveBank", a2: &unk_18001C240, a3: &phwi[6]); /*0x1800096bd*/
            v4 = -2147467259; /*0x1800096c3*/
            goto LABEL_38; /*0x1800096c9*/
          }
          v4 = sub_180006180(a1: (char *)&unk_188F75A30 + 704 * (int)v11, a2: v11, a3: &phwi[6]); /*0x1800096e7*/
          v22 = 704 * v12; /*0x1800096ea*/
          v1 = this; /*0x1800096f1*/
          if ( v4 != 0 ) /*0x1800096f8*/
            *(_DWORD *)((char *)&unk_188F75920 + v22 + 4) = 7; /*0x180009705*/
          else
            *(_DWORD *)((char *)&unk_188F75920 + v22 + 4) = 6; /*0x1800096fa*/
LABEL_39:
          v10 = v1; /*0x180009715*/
          if ( LOBYTE(phwi[1]) == 0 ) /*0x18000971d*/
          {
LABEL_13:
            CLibXact::del_cue(this: v10); /*0x180009507*/
            goto LABEL_193; /*0x18000950c*/
          }
          v9 = v4; /*0x180009723*/
          goto LABEL_192; /*0x180009726*/
        case 17: /*0x180009457*/
          v23 = 0; /*0x180009744*/
          v24 = 0; /*0x180009747*/
          if ( byte_180038820 != 0 ) /*0x180009751*/
          {
            avs2_core_382(a1: "CLibXact::LoadSoundBank", a2: &unk_18001C1C8); /*0x180009761*/
            byte_180038820 = 0; /*0x180009767*/
            v4 = 0; /*0x18000976e*/
            goto LABEL_66; /*0x180009771*/
          }
          v25 = 0; /*0x180009776*/
          v26 = 0; /*0x180009778*/
          v27 = (int *)&unk_180046830; /*0x18000977a*/
          do /*0x1800097bd*/
          {
            if ( v27[1] >= 0 /*0x1800097ac*/
              && *v27 >= 0
              && (unsigned int)avs2_core_229(a1: (char *)&unk_18004693C + 788 * v25, a2: &phwi[6]) == 0 )
            {
              avs2_core_244(a1: (char *)&unk_180046830 + 788 * v25, a2: 0, a3: 788); /*0x1800097dd*/
              *(_QWORD *)((char *)&unk_180046830 + 788 * v26) = -1; /*0x1800097ea*/
              avs2_core_382(a1: "CLibXact::LoadSoundBank", a2: &unk_18001C1C8); /*0x180009800*/
              byte_180038820 = 1; /*0x180009806*/
              v4 = 0; /*0x18000980d*/
LABEL_65:
              v1 = this; /*0x180009944*/
              goto LABEL_66; /*0x180009944*/
            }
            ++v25; /*0x1800097ae*/
            ++v26; /*0x1800097b0*/
            v27 += 197; /*0x1800097b3*/
          }
          while ( v25 < 64 ); /*0x1800097bd*/
          v28 = (char *)&unk_188F8B928; /*0x18000981c*/
          for ( j = 128; j != 0; --j ) /*0x18000981f*/
          {
            if ( (unsigned int)avs2_core_229(a1: v28, a2: &phwi[6]) == 0 ) /*0x18000983f*/
            {
              avs2_core_380(a1: "CLibXact::LoadSoundBank", a2: &unk_18001C280, a3: &phwi[6]); /*0x180009853*/
              v4 = -2147467259; /*0x180009859*/
              v24 = 1; /*0x18000985f*/
            }
            v28 += 50592; /*0x180009865*/
          }
          if ( v24 != 0 ) /*0x180009874*/
            goto LABEL_65; /*0x180009874*/
          v31 = 0; /*0x18000987a*/
          v32 = &unk_188F8B920; /*0x18000987c*/
          while ( *v32 != -1 ) /*0x180009889*/
          {
            ++v31; /*0x18000988b*/
            ++j; /*0x18000988d*/
            v32 += 12648; /*0x180009890*/
            if ( (__int64)v32 >= (__int64)&qword_1895B8920 ) /*0x180009899*/
              goto LABEL_60; /*0x180009899*/
          }
          v23 = v31; /*0x18000989d*/
          v33 = 50592 * j; /*0x1800098a0*/
          *(_DWORD *)((char *)&unk_188F8B920 + v33) = v31; /*0x1800098a7*/
          avs2_core_223(a1: (char *)&unk_188F8B928 + 50592 * (int)v31, a2: &phwi[6], a3: 260); /*0x1800098c2*/
          *(_DWORD *)((char *)&unk_188F8B920 + v33 + 4) = 5; /*0x1800098c8*/
LABEL_60:
          if ( v31 == 128 ) /*0x1800098d7*/
          {
            avs2_core_380(a1: "CLibXact::LoadSoundBank", a2: &unk_18001C2A8, a3: &phwi[6]); /*0x1800098eb*/
            v4 = -2147467259; /*0x1800098f1*/
            goto LABEL_65; /*0x1800098f7*/
          }
          sub_180008C40(a1: (unsigned __int16)v23, a2: v30); /*0x1800098fd*/
          v34 = 50592LL * (int)v23; /*0x180009905*/
          v4 = sub_180004490(a1: (char *)&unk_188F8BA30 + v34, a2: v23, a3: &phwi[6]); /*0x180009922*/
          v1 = this; /*0x180009925*/
          if ( v4 != 0 ) /*0x18000992c*/
            *(_DWORD *)((char *)&unk_188F8B920 + v34 + 4) = 7; /*0x180009939*/
          else
            *(_DWORD *)((char *)&unk_188F8B920 + v34 + 4) = 6; /*0x18000992e*/
LABEL_66:
          if ( LOBYTE(phwi[1]) != 0 ) /*0x180009951*/
            CLibXact::FinishCommand(this: v1, a2: v4); /*0x180009956*/
          else
LABEL_107:
            CLibXact::del_cue(this: v1); /*0x180009eed*/
          goto LABEL_193; /*0x180009962*/
        case 18: /*0x180009457*/
          v35 = SLODWORD(phwi[2]); /*0x180009978*/
          v36 = 704LL * SLODWORD(phwi[2]); /*0x180009980*/
          *(_DWORD *)((char *)&unk_188F75920 + v36) = -1; /*0x18000998e*/
          *(_DWORD *)((char *)&unk_188F75920 + v36 + 4) = 4; /*0x180009996*/
          avs2_core_244(a1: (char *)&unk_188F75928 + v36, a2: 0, a3: 260); /*0x1800099ae*/
          sub_1800064A0(a1: (char *)&unk_188F75A30 + 704 * v35); /*0x1800099c5*/
          v10 = v1; /*0x1800099ca*/
          if ( LOBYTE(phwi[1]) == 0 ) /*0x1800099d2*/
            goto LABEL_107; /*0x1800099d2*/
          goto LABEL_191; /*0x1800099d2*/
        case 19: /*0x180009457*/
          v37 = SLODWORD(phwi[2]); /*0x1800099e2*/
          sub_180008C40(a1: LOWORD(phwi[2]), a2: 0x180000000uLL); /*0x1800099ec*/
          v37 *= 50592; /*0x1800099f1*/
          *(_DWORD *)((char *)&unk_188F8B920 + v37) = -1; /*0x1800099ff*/
          *(_DWORD *)((char *)&unk_188F8B920 + v37 + 4) = 4; /*0x180009a07*/
          avs2_core_244(a1: (char *)&unk_188F8B928 + v37, a2: 0, a3: 260); /*0x180009a1f*/
          sub_180004EE0(a1: (char *)&unk_188F8BA30 + v37); /*0x180009a2f*/
          if ( LOBYTE(phwi[1]) == 0 ) /*0x180009a3c*/
            goto LABEL_107; /*0x180009a3c*/
          CLibXact::FinishCommand(this: v1, a2: 0); /*0x180009a40*/
          goto LABEL_193; /*0x180009a4c*/
        case 20: /*0x180009457*/
          v42 = (int)phwi[4]; /*0x180009dce*/
          v43 = HIDWORD(phwi[4]); /*0x180009dd5*/
          v44 = phwi[5]; /*0x180009ddc*/
          if ( phwi[5] == nullptr ) /*0x180009de3*/
          {
            v44 = (HWAVEIN)sub_180005E90(Src: &phwi[6]); /*0x180009de9*/
            if ( v44 == nullptr ) /*0x180009df1*/
              goto LABEL_104; /*0x180009df1*/
          }
          v45 = sub_180004870( /*0x180009e17*/
                  a1: (unsigned int)&unk_188F8BA30 + 50592 * *(_WORD *)v44,
                  a2: (_DWORD)v44,
                  a3: v5,
                  a4: v6,
                  a5: v42,
                  a6: v43);
          goto LABEL_103; /*0x180009e17*/
        case 21: /*0x180009457*/
          v92 = phwi[5]; /*0x18000a458*/
          if ( phwi[5] == nullptr ) /*0x18000a45f*/
          {
            v92 = (HWAVEIN)sub_180005E90(Src: &phwi[6]); /*0x18000a465*/
            if ( v92 == nullptr ) /*0x18000a46d*/
              goto LABEL_104; /*0x18000a46d*/
          }
          v45 = sub_1800052A0(a1: (char *)&unk_188F8BA30 + 50592 * *(_WORD *)v92, a2: *((unsigned __int16 *)v92 + 1)); /*0x18000a484*/
          goto LABEL_103; /*0x18000a489*/
        case 22: /*0x180009457*/
          v93 = phwi[5]; /*0x18000a48e*/
          if ( phwi[5] == nullptr ) /*0x18000a495*/
          {
            v93 = (HWAVEIN)sub_180005E90(Src: &phwi[6]); /*0x18000a49b*/
            if ( v93 == nullptr ) /*0x18000a4a3*/
              goto LABEL_104; /*0x18000a4a3*/
          }
          v45 = sub_180003F40(a1: (char *)&unk_188F8BA30 + 50592 * *(_WORD *)v93, a2: *((unsigned __int16 *)v93 + 1)); /*0x18000a4ba*/
LABEL_103:
          v4 = v45; /*0x180009e1c*/
LABEL_104:
          v10 = v1; /*0x180009e1f*/
          if ( LOBYTE(phwi[1]) == 0 ) /*0x180009e27*/
            goto LABEL_13; /*0x180009e27*/
          v9 = v4; /*0x180009e2d*/
          goto LABEL_192; /*0x180009e30*/
        case 23: /*0x180009457*/
          v94 = phwi[5]; /*0x18000a4ca*/
          if ( phwi[5] != nullptr || (v94 = (HWAVEIN)sub_180005E90(Src: &phwi[6])) != nullptr ) /*0x18000a4df*/
            sub_180003E60( /*0x18000a4fc*/
              a1: (char *)&unk_188F8BA30 + 50592 * *(_WORD *)v94,
              a2: *((unsigned __int16 *)v94 + 1),
              a3: (char *)&phwi[38] + 4);
          goto LABEL_107; /*0x18000a504*/
        case 24: /*0x180009457*/
          (*(void (__fastcall **)(__int64, char *))(*(_QWORD *)qword_188F75800 + 168LL))( /*0x18000a525*/
            a1: qword_188F75800,
            a2: (char *)&phwi[38] + 4);
          (*(void (__fastcall **)(__int64, _QWORD))(*(_QWORD *)qword_188F75800 + 176LL))(a1: qword_188F75800, a2: v95); /*0x18000a53b*/
          CLibXact::del_cue(this: v1); /*0x18000a545*/
          goto LABEL_193; /*0x18000a54a*/
        case 25: /*0x180009457*/
          sub_180003510(a1: &unk_188F5FEC0, a2: 0x180000000uLL); /*0x180009da7*/
          goto LABEL_190; /*0x180009dac*/
        case 26: /*0x180009457*/
          sub_1800011B0(a1: &unk_188F5FEC0, a2: 0x180000000uLL); /*0x180009db8*/
          goto LABEL_190; /*0x180009dbd*/
        case 27: /*0x180009457*/
          v96 = (unsigned __int16 *)sub_180005E90(Src: &phwi[6]); /*0x18000a553*/
          if ( v96 != nullptr ) /*0x18000a55b*/
            v4 = sub_1800041B0(a1: (char *)&unk_188F8BA30 + 50592 * *v96, a2: v96[1]); /*0x18000a573*/
          goto LABEL_165; /*0x18000a573*/
        case 28: /*0x180009457*/
          v97 = (unsigned __int16 *)sub_180005E90(Src: &phwi[6]); /*0x18000a582*/
          if ( v97 != nullptr ) /*0x18000a58a*/
            v4 = (int)sub_180004040(a1: (char *)&unk_188F8BA30 + 50592 * *v97, a2: v97[1]); /*0x18000a5a2*/
          v9 = v4; /*0x18000a5a7*/
          goto LABEL_76; /*0x18000a5aa*/
        case 29: /*0x180009457*/
          LODWORD(phwi[0]) = 0; /*0x18000a5af*/
          v98 = (unsigned __int16 *)sub_180005E90(Src: &phwi[6]); /*0x18000a5bb*/
          if ( v98 != nullptr ) /*0x18000a5c3*/
            v9 = sub_180004310(a1: (char *)&unk_188F8BA30 + 50592 * *v98, a2: v98[1], a3: phwi); /*0x18000a5e3*/
          else
LABEL_165:
            v9 = v4; /*0x18000a576*/
          goto LABEL_76; /*0x18000a5e5*/
        case 30: /*0x180009457*/
          v38 = 0; /*0x18000a5ea*/
          memset(a1: &unk_180032820, Val: 0, Size: 0x6000u); /*0x18000a5fb*/
          v99 = &unk_180032820; /*0x18000a600*/
          v100 = &unk_180038838; /*0x18000a607*/
          do /*0x18000a669*/
          {
            if ( *((_QWORD *)v100 - 1) != 0 && *(_WORD *)v100 != 0xFFFF ) /*0x18000a62b*/
            {
              *v99 = *v100; /*0x18000a62f*/
              v99[1] = v100[1]; /*0x18000a635*/
              v99[2] = v100[2]; /*0x18000a63c*/
              v99[3] = v100[3]; /*0x18000a643*/
              v99[4] = v100[4]; /*0x18000a64a*/
              v99[5] = v100[5]; /*0x18000a651*/
              ++v38; /*0x18000a655*/
              v99 += 6; /*0x18000a657*/
            }
            v100 += 14; /*0x18000a662*/
          }
          while ( (__int64)v100 < (__int64)&unk_180046838 ); /*0x18000a669*/
          goto LABEL_75; /*0x18000a669*/
        case 31: /*0x180009457*/
          v101 = 0; /*0x18000a677*/
          v102 = (unsigned int)phwi[2]; /*0x18000a679*/
          v103 = SHIDWORD(phwi[2]); /*0x18000a67e*/
          memset(a1: Destination, Val: 0, Size: sizeof(Destination)); /*0x18000a692*/
          v104 = 0; /*0x18000a697*/
          v105 = 0; /*0x18000a699*/
          while ( (unsigned int)avs2_core_229(a1: (char *)&unk_188F8B928 + 50592 * v104, a2: (char *)&phwi[38] + 4) != 0 ) /*0x18000a6c3*/
          {
            ++v104; /*0x18000a6c5*/
            ++v105; /*0x18000a6c7*/
            if ( v104 >= 128 ) /*0x18000a6d0*/
              goto LABEL_182; /*0x18000a6d0*/
          }
          v106 = 50592 * v105; /*0x18000a6d4*/
          v107 = 50592LL * *(int *)((char *)&unk_188F8B920 + v106); /*0x18000a6e6*/
          v108 = *(unsigned __int16 *)((char *)&unk_188F8B920 + v107 + 274); /*0x18000a6ed*/
          if ( *(_WORD *)((char *)&unk_188F8B920 + v107 + 274) != 0 ) /*0x18000a6f9*/
          {
            v109 = 0; /*0x18000a704*/
            while ( 1 ) /*0x18000a720*/
            {
              memset(Buffer, 0, 255); /*0x18000a720*/
              sub_180003FC0( /*0x18000a761*/
                a1: (char *)&unk_188F8BA30 + 50592 * *(int *)((char *)&unk_188F8B920 + v106),
                a2: (unsigned __int16)v109,
                a3: Buffer);
              v101 += avs2_core_222(a1: Buffer) + 1; /*0x18000a775*/
              if ( v101 >= (int)(v102 - 1) ) /*0x18000a77a*/
                break; /*0x18000a77a*/
              strncat(Destination: Destination, Source: Buffer, Count: 0xFFu); /*0x18000a794*/
              strncat(Destination: Destination, Source: ",", Count: 1u); /*0x18000a7ad*/
              if ( ++v109 >= v108 ) /*0x18000a7b7*/
              {
                avs2_core_223(a1: v103, a2: Destination, a3: v102); /*0x18000a7d0*/
                goto LABEL_187; /*0x18000a7d0*/
              }
            }
          }
LABEL_182:
          v108 = -1; /*0x18000a6fb*/
LABEL_187:
          v9 = v108; /*0x18000a7d6*/
          v1 = this; /*0x18000a7d9*/
          v10 = this; /*0x18000a7de*/
          goto LABEL_192; /*0x18000a7e1*/
        case 32: /*0x180009457*/
          v46 = (int)phwi[4]; /*0x180009e41*/
          v47 = HIDWORD(phwi[4]); /*0x180009e48*/
          v112 = 0xFFFF; /*0x180009e54*/
          memset(Buffer, 0, 260); /*0x180009e62*/
          memset(v118, 0, 260); /*0x180009e7d*/
          sprintf(Buffer, Format: "%s.xwb", (const char *)&phwi[38] + 4); /*0x180009ead*/
          sprintf(Buffer: v118, Format: "%s.xsb", (const char *)&phwi[38] + 4); /*0x180009ec7*/
          sub_180007CD0(); /*0x180009ecc*/
          v48 = avs2_core_79(a1: Buffer, a2: 1, a3: 511); /*0x180009ee3*/
          if ( v48 < 0 ) /*0x180009eeb*/
            goto LABEL_107; /*0x180009eeb*/
          avs2_core_86(a1: (unsigned int)v48); /*0x180009efc*/
          v49 = avs2_core_79(a1: v118, a2: 1, a3: 511); /*0x180009f14*/
          if ( v49 < 0 ) /*0x180009f1c*/
            goto LABEL_107; /*0x180009f1c*/
          avs2_core_86(a1: (unsigned int)v49); /*0x180009f2d*/
          v50 = (int *)&unk_180046834; /*0x180009f33*/
          v51 = (char *)&unk_180046838; /*0x180009f3a*/
          for ( k = 64; k != 0; --k ) /*0x180009f41*/
          {
            if ( (unsigned int)avs2_core_229(a1: v51 + 260, a2: v118) == 0 /*0x180009f8e*/
              && (unsigned int)avs2_core_229(a1: v51, a2: Buffer) == 0 )
            {
              v53 = 50592LL * *v50; /*0x180009f97*/
              v54 = *(unsigned __int16 *)((char *)&unk_188F8B920 + v53 + 274); /*0x180009f9e*/
              v55 = 0; /*0x180009fa7*/
              if ( *(_WORD *)((char *)&unk_188F8B920 + v53 + 274) != 0 ) /*0x180009fab*/
              {
                do /*0x180009fc9*/
                  sub_1800052A0(a1: (char *)&unk_188F8BA30 + 50592 * *v50, a2: (unsigned __int16)v55++); /*0x180009fc0*/
                while ( v55 < v54 ); /*0x180009fc9*/
              }
              sub_180004EE0(a1: (char *)&unk_188F8BA30 + 50592 * *v50); /*0x180009fd8*/
              sub_1800064A0(a1: (char *)&unk_188F75A30 + 704 * *(v50 - 1)); /*0x180009ff2*/
              avs2_core_244(a1: (char *)&unk_188F75920 + 704 * *(v50 - 1), a2: 0, a3: 704); /*0x18000a014*/
              v56 = 704LL * *(v50 - 1); /*0x18000a01e*/
              *(_DWORD *)((char *)&unk_188F75920 + v56) = -1; /*0x18000a025*/
              *(_DWORD *)((char *)&unk_188F75920 + v56 + 4) = 4; /*0x18000a02c*/
              avs2_core_244(a1: (char *)&unk_188F8B920 + 50592 * *v50, a2: 0, a3: 50592); /*0x18000a049*/
              v57 = 50592LL * *v50; /*0x18000a052*/
              *(_DWORD *)((char *)&unk_188F8B920 + v57) = -1; /*0x18000a059*/
              *(_DWORD *)((char *)&unk_188F8B920 + v57 + 4) = 4; /*0x18000a061*/
              avs2_core_244(a1: v51 - 8, a2: 0, a3: 788); /*0x18000a076*/
              *(_QWORD *)(v50 - 1) = -1; /*0x18000a07c*/
            }
            v51 += 788; /*0x18000a084*/
            v50 += 197; /*0x18000a08b*/
          }
          v58 = 0xFFFF; /*0x18000a09b*/
          v59 = 0; /*0x18000a0a0*/
          v60 = 0; /*0x18000a0a2*/
          v61 = -1; /*0x18000a0a4*/
          v62 = &unk_188F75920; /*0x18000a0a7*/
          do /*0x18000a0e9*/
          {
            if ( *v62 == -1 ) /*0x18000a0b8*/
              break; /*0x18000a0b8*/
            if ( (unsigned int)avs2_core_229(a1: (char *)&unk_188F75928 + 704 * v60, a2: Buffer) == 0 ) /*0x18000a0d6*/
              ++v59; /*0x18000a0d8*/
            ++v60; /*0x18000a0da*/
            v62 += 176; /*0x18000a0dc*/
          }
          while ( v60 < 128 ); /*0x18000a0e9*/
          v63 = 0; /*0x18000a0eb*/
          v64 = 0xFFFF; /*0x18000a0ed*/
          do /*0x18000a137*/
          {
            v65 = 50592LL * v63; /*0x18000a103*/
            if ( *(_DWORD *)((char *)&unk_188F8B920 + v65) == -1 ) /*0x18000a10f*/
              break; /*0x18000a10f*/
            if ( (unsigned int)avs2_core_229(a1: (char *)&unk_188F8B928 + v65, a2: v118) == 0 ) /*0x18000a127*/
            {
              ++v59; /*0x18000a129*/
              v61 = v63; /*0x18000a12b*/
            }
            ++v63; /*0x18000a12f*/
          }
          while ( v63 < 128 ); /*0x18000a137*/
          if ( v59 == 1 ) /*0x18000a13c*/
          {
            v1 = this; /*0x18000a13e*/
            v10 = this; /*0x18000a143*/
            goto LABEL_13; /*0x18000a146*/
          }
          if ( v59 == 2 ) /*0x18000a153*/
          {
            v66 = sub_180005E90(Src: &phwi[6]); /*0x18000a159*/
            sub_180004870(a1: (unsigned int)&unk_188F8BA30 + 50592 * v61, a2: v66, a3: v67, a4: v68, a5: v46, a6: v47); /*0x18000a18a*/
            v1 = this; /*0x18000a18f*/
            CLibXact::del_cue(this); /*0x18000a197*/
            goto LABEL_193; /*0x18000a19c*/
          }
          v69 = 0; /*0x18000a1a1*/
          v70 = 0; /*0x18000a1a3*/
          v71 = &unk_188F75920; /*0x18000a1ac*/
          while ( *v71 != -1 ) /*0x18000a1b9*/
          {
            ++v69; /*0x18000a1bb*/
            ++v70; /*0x18000a1bd*/
            v71 += 176; /*0x18000a1c0*/
            if ( (__int64)v71 >= (__int64)&unk_188F8B920 ) /*0x18000a1c9*/
            {
              v72 = 0xFFFF; /*0x18000a1cb*/
              goto LABEL_135; /*0x18000a1ce*/
            }
          }
          v73 = 704 * v70; /*0x18000a1d3*/
          *(_DWORD *)((char *)&unk_188F75920 + v73) = v69; /*0x18000a1da*/
          *(_DWORD *)((char *)&unk_188F75920 + v73 + 4) = 6; /*0x18000a1dd*/
          avs2_core_223(a1: (char *)&unk_188F75928 + 704 * v69, a2: Buffer, a3: 260); /*0x18000a206*/
          v72 = v69; /*0x18000a20c*/
          v112 = v69; /*0x18000a20f*/
          v64 = v70; /*0x18000a213*/
LABEL_135:
          v74 = 0; /*0x18000a216*/
          v75 = 0; /*0x18000a218*/
          v76 = &unk_188F8B920; /*0x18000a21a*/
          while ( *v76 != -1 ) /*0x18000a227*/
          {
            ++v74; /*0x18000a229*/
            ++v75; /*0x18000a22b*/
            v76 += 12648; /*0x18000a22e*/
            if ( (__int64)v76 >= (__int64)&qword_1895B8920 ) /*0x18000a237*/
            {
              v77 = 0xFFFF; /*0x18000a239*/
              goto LABEL_140; /*0x18000a23c*/
            }
          }
          v78 = 50592 * v75; /*0x18000a241*/
          *(_DWORD *)((char *)&unk_188F8B920 + v78) = v74; /*0x18000a248*/
          *(_DWORD *)((char *)&unk_188F8B920 + v78 + 4) = 6; /*0x18000a24c*/
          avs2_core_223(a1: (char *)&unk_188F8B928 + 50592 * v74, a2: v118, a3: 260); /*0x18000a276*/
          v77 = v74; /*0x18000a27c*/
          v58 = v75; /*0x18000a27f*/
LABEL_140:
          v79 = 704LL * v72; /*0x18000a282*/
          v80 = 50592LL * v77; /*0x18000a2b2*/
          if ( (unsigned int)sub_180006180(a1: (char *)&unk_188F75A30 + v79, a2: (unsigned int)v72, a3: Buffer) == -2147467259 ) /*0x18000a2be*/
          {
            sub_180004EE0(a1: (char *)&unk_188F8BA30 + v80); /*0x18000a2ce*/
LABEL_142:
            avs2_core_244(a1: (char *)&unk_188F8B920 + v80, a2: 0, a3: 50592); /*0x18000a2d3*/
            v81 = 50592 * v58; /*0x18000a2e5*/
            *(_DWORD *)((char *)&unk_188F8B920 + v81) = -1; /*0x18000a2ec*/
            *(_DWORD *)((char *)&unk_188F8B920 + v81 + 4) = 4; /*0x18000a2f5*/
            sub_1800064A0(a1: (char *)&unk_188F75A30 + v79); /*0x18000a301*/
            avs2_core_244(a1: (char *)&unk_188F75920 + v79, a2: 0, a3: 704); /*0x18000a319*/
            v82 = 704 * v64; /*0x18000a31f*/
            *(_DWORD *)((char *)&unk_188F75920 + v82) = -1; /*0x18000a326*/
            *(_DWORD *)((char *)&unk_188F75920 + v82 + 4) = 4; /*0x18000a32e*/
            goto LABEL_143; /*0x18000a32e*/
          }
          v83 = (char *)&unk_188F8BA30 + v80; /*0x18000a350*/
          if ( (unsigned int)sub_180004490(a1: (char *)&unk_188F8BA30 + v80, a2: (unsigned int)v77, a3: v118) == -2147467259 ) /*0x18000a36a*/
          {
            sub_180004EE0(a1: v83); /*0x18000a36f*/
            goto LABEL_142; /*0x18000a37b*/
          }
          v84 = 0; /*0x18000a380*/
          v85 = 0; /*0x18000a382*/
          v86 = &unk_180046834; /*0x18000a384*/
          while ( *v86 != -1 ) /*0x18000a395*/
          {
            ++v84; /*0x18000a397*/
            ++v85; /*0x18000a399*/
            v86 += 197; /*0x18000a39c*/
            if ( (__int64)v86 >= (__int64)dword_180052D34 ) /*0x18000a3a5*/
              goto LABEL_151; /*0x18000a3a5*/
          }
          v87 = 788 * v85; /*0x18000a3a9*/
          *(_DWORD *)((char *)&unk_180046830 + v87 + 4) = v77; /*0x18000a3b7*/
          *(_DWORD *)((char *)&unk_180046830 + v87) = v112; /*0x18000a3c0*/
          v88 = 788LL * v84; /*0x18000a3c7*/
          avs2_core_223(a1: (char *)&unk_180046838 + v88, a2: Buffer, a3: 260); /*0x18000a3e5*/
          avs2_core_223(a1: (char *)&unk_18004693C + v88, a2: v118, a3: 260); /*0x18000a402*/
          avs2_core_223(a1: (char *)&unk_180046A40 + v88, a2: &phwi[6], a3: 260); /*0x18000a41c*/
LABEL_151:
          v89 = sub_180005E90(Src: &phwi[6]); /*0x18000a422*/
          if ( v89 != 0 ) /*0x18000a42e*/
            sub_180004870(a1: (_DWORD)v83, a2: v89, a3: v90, a4: v91, a5: v46, a6: v47); /*0x18000a44e*/
LABEL_143:
          v1 = this; /*0x18000a337*/
          CLibXact::del_cue(this); /*0x18000a33f*/
          goto LABEL_193; /*0x18000a344*/
        case 33: /*0x180009457*/
          if ( LODWORD(phwi[2]) == 36 ) /*0x18000a7eb*/
            avs2_core_58(a1: &unk_180052D30, a2: 0, a3: phwi[6]); /*0x18000a7fa*/
          goto LABEL_190; /*0x18000a7fa*/
        default:
          goto LABEL_193;
      }
      break;
    }
  }
}
