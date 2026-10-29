__int64 __fastcall sub_1802FBEF0(__int64 *a1, __int64 a2)
{
  __int64 result; // rax
  int v5; // ebp
  int v6; // ebx
  __int64 v7; // rsi
  int v8; // eax
  __int64 v9; // r14
  __int64 v10; // rcx
  __int64 v11; // rbx
  __int64 v12; // rcx
  _DWORD *v13; // rcx
  __int64 (__fastcall *v14)(); // rdx
  int v15; // eax
  __int64 v16; // rbx
  __int64 *v17; // rsi
  __int64 v18; // rdx
  int v19; // ebp
  int v20; // ebx
  __int64 v21; // rsi
  int v22; // eax
  __int64 v23; // r14
  __int64 v24; // rcx
  int v25; // ebp
  int v26; // ebx
  __int64 v27; // rsi
  int v28; // eax
  __int64 v29; // r14
  __int64 v30; // rcx
  int v31; // ebp
  int v32; // ebx
  __int64 v33; // rsi
  int v34; // eax
  __int64 v35; // r14
  __int64 v36; // rcx
  int v37; // ebp
  int v38; // ebx
  __int64 v39; // rsi
  int v40; // eax
  __int64 v41; // r14
  __int64 v42; // rcx
  int v43; // ebp
  int v44; // ebx
  __int64 v45; // rsi
  int v46; // eax
  __int64 v47; // r14
  __int64 v48; // rcx
  _DWORD *v49; // rcx
  __int64 (__fastcall *v50)(); // rdx
  int v51; // eax
  __int64 v52; // rbx
  __int64 *v53; // rsi
  __int64 v54; // rax
  _DWORD *v55; // [rsp+20h] [rbp-28h] BYREF
  int v56; // [rsp+28h] [rbp-20h]

  result = *(int *)(a2 + 8); /*0x1802fbef8*/
  switch ( *(_DWORD *)(a2 + 8) ) /*0x1802fbf31*/
  {
    case 0: /*0x1802fbf31*/
      v5 = *((_DWORD *)a1 + 16); /*0x1802fbf34*/
      v6 = 0; /*0x1802fbf38*/
      result = v5; /*0x1802fbf3a*/
      while ( 1 ) /*0x1802fbf44*/
      {
        v7 = 0; /*0x1802fbf44*/
        if ( v5 < 256 ) /*0x1802fbf4c*/
          v7 = result; /*0x1802fbf4c*/
        v8 = 0; /*0x1802fbf50*/
        if ( v5 < 256 ) /*0x1802fbf58*/
          v8 = v5; /*0x1802fbf58*/
        v9 = v8; /*0x1802fbf5b*/
        v5 = v8 + 1; /*0x1802fbf65*/
        if ( (*(unsigned __int8 (__fastcall **)(__int64))(*(_QWORD *)(216 * v7 + a1[2]) + 128LL))(a1: a1[2] + 216LL * v8) == 0 ) /*0x1802fbf7e*/
          break; /*0x1802fbf7e*/
        ++v6; /*0x1802fbf80*/
        result = v7 + 1; /*0x1802fbf82*/
        if ( v6 >= 256 ) /*0x1802fbf8c*/
          return result; /*0x1802fbf8c*/
      }
      v10 = a1[2]; /*0x1802fbf93*/
      v11 = 216 * v9; /*0x1802fbf9a*/
      *((_DWORD *)a1 + 16) = v5; /*0x1802fbfa1*/
      (*(void (__fastcall **)(__int64, __int64))(*(_QWORD *)(216 * v9 + v10) + 8LL))(a1: 216 * v9 + v10, a2); /*0x1802fbfaa*/
      v12 = a1[2]; /*0x1802fbfad*/
      goto LABEL_11; /*0x1802fbfad*/
    case 1: /*0x1802fbf31*/
      v19 = *((_DWORD *)a1 + 17); /*0x1802fc020*/
      v20 = 0; /*0x1802fc024*/
      result = v19; /*0x1802fc026*/
      while ( 1 ) /*0x1802fc034*/
      {
        v21 = 0; /*0x1802fc034*/
        if ( v19 < 128 ) /*0x1802fc03c*/
          v21 = result; /*0x1802fc03c*/
        v22 = 0; /*0x1802fc040*/
        if ( v19 < 128 ) /*0x1802fc048*/
          v22 = v19; /*0x1802fc048*/
        v23 = v22; /*0x1802fc04b*/
        v19 = v22 + 1; /*0x1802fc055*/
        if ( (*(unsigned __int8 (__fastcall **)(__int64))(*(_QWORD *)(472 * v21 + a1[3]) + 128LL))(a1: a1[3] + 472LL * v22) == 0 ) /*0x1802fc06e*/
          break; /*0x1802fc06e*/
        ++v20; /*0x1802fc070*/
        result = v21 + 1; /*0x1802fc072*/
        if ( v20 >= 128 ) /*0x1802fc07c*/
          return result; /*0x1802fc07c*/
      }
      v24 = a1[3]; /*0x1802fc083*/
      v11 = 472 * v23; /*0x1802fc08a*/
      *((_DWORD *)a1 + 17) = v19; /*0x1802fc091*/
      (*(void (__fastcall **)(__int64, __int64))(*(_QWORD *)(472 * v23 + v24) + 8LL))(a1: 472 * v23 + v24, a2); /*0x1802fc09a*/
      v12 = a1[3]; /*0x1802fc09d*/
      goto LABEL_11; /*0x1802fc0a1*/
    case 2: /*0x1802fbf31*/
      v25 = *((_DWORD *)a1 + 18); /*0x1802fc0a6*/
      v26 = 0; /*0x1802fc0aa*/
      result = v25; /*0x1802fc0ac*/
      while ( 1 ) /*0x1802fc0b4*/
      {
        v27 = 0; /*0x1802fc0b4*/
        if ( v25 < 256 ) /*0x1802fc0bc*/
          v27 = result; /*0x1802fc0bc*/
        v28 = 0; /*0x1802fc0c0*/
        if ( v25 < 256 ) /*0x1802fc0c8*/
          v28 = v25; /*0x1802fc0c8*/
        v29 = v28; /*0x1802fc0cb*/
        v25 = v28 + 1; /*0x1802fc0d5*/
        if ( (*(unsigned __int8 (__fastcall **)(__int64))(*(_QWORD *)(280 * v27 + a1[4]) + 128LL))(a1: a1[4] + 280LL * v28) == 0 ) /*0x1802fc0ee*/
          break; /*0x1802fc0ee*/
        ++v26; /*0x1802fc0f0*/
        result = v27 + 1; /*0x1802fc0f2*/
        if ( v26 >= 256 ) /*0x1802fc0fc*/
          return result; /*0x1802fc0fc*/
      }
      v30 = a1[4]; /*0x1802fc103*/
      v11 = 280 * v29; /*0x1802fc10a*/
      *((_DWORD *)a1 + 18) = v25; /*0x1802fc111*/
      (*(void (__fastcall **)(__int64, __int64))(*(_QWORD *)(280 * v29 + v30) + 8LL))(a1: 280 * v29 + v30, a2); /*0x1802fc11a*/
      v12 = a1[4]; /*0x1802fc11d*/
      goto LABEL_11; /*0x1802fc121*/
    case 3: /*0x1802fbf31*/
      v31 = *((_DWORD *)a1 + 19); /*0x1802fc126*/
      v32 = 0; /*0x1802fc12a*/
      result = v31; /*0x1802fc12c*/
      while ( 1 ) /*0x1802fc134*/
      {
        v33 = 0; /*0x1802fc134*/
        if ( v31 < 32 ) /*0x1802fc139*/
          v33 = result; /*0x1802fc139*/
        v34 = 0; /*0x1802fc13d*/
        if ( v31 < 32 ) /*0x1802fc142*/
          v34 = v31; /*0x1802fc142*/
        v35 = v34; /*0x1802fc145*/
        v31 = v34 + 1; /*0x1802fc14f*/
        if ( (*(unsigned __int8 (__fastcall **)(__int64))(*(_QWORD *)(216 * v33 + a1[5]) + 128LL))(a1: a1[5] + 216LL * v34) == 0 ) /*0x1802fc168*/
          break; /*0x1802fc168*/
        ++v32; /*0x1802fc16a*/
        result = v33 + 1; /*0x1802fc16c*/
        if ( v32 >= 32 ) /*0x1802fc173*/
          return result; /*0x1802fc173*/
      }
      v36 = a1[5]; /*0x1802fc17a*/
      v11 = 216 * v35; /*0x1802fc181*/
      *((_DWORD *)a1 + 19) = v31; /*0x1802fc188*/
      (*(void (__fastcall **)(__int64, __int64))(*(_QWORD *)(216 * v35 + v36) + 8LL))(a1: 216 * v35 + v36, a2); /*0x1802fc191*/
      v12 = a1[5]; /*0x1802fc194*/
      goto LABEL_11; /*0x1802fc198*/
    case 4: /*0x1802fbf31*/
      v37 = *((_DWORD *)a1 + 20); /*0x1802fc19d*/
      v38 = 0; /*0x1802fc1a1*/
      result = v37; /*0x1802fc1a3*/
      while ( 1 ) /*0x1802fc1b4*/
      {
        v39 = 0; /*0x1802fc1b4*/
        if ( v37 < 16 ) /*0x1802fc1b9*/
          v39 = result; /*0x1802fc1b9*/
        v40 = 0; /*0x1802fc1bd*/
        if ( v37 < 16 ) /*0x1802fc1c2*/
          v40 = v37; /*0x1802fc1c2*/
        v41 = v40; /*0x1802fc1c5*/
        v37 = v40 + 1; /*0x1802fc1cf*/
        if ( (*(unsigned __int8 (__fastcall **)(__int64))(*(_QWORD *)(480 * v39 + a1[6]) + 128LL))(a1: a1[6] + 480LL * v40) == 0 ) /*0x1802fc1e8*/
          break; /*0x1802fc1e8*/
        ++v38; /*0x1802fc1ea*/
        result = v39 + 1; /*0x1802fc1ec*/
        if ( v38 >= 16 ) /*0x1802fc1f3*/
          return result; /*0x1802fc1f3*/
      }
      v42 = a1[6]; /*0x1802fc1fa*/
      v11 = 480 * v41; /*0x1802fc201*/
      *((_DWORD *)a1 + 20) = v37; /*0x1802fc208*/
      (*(void (__fastcall **)(__int64, __int64))(*(_QWORD *)(480 * v41 + v42) + 8LL))(a1: 480 * v41 + v42, a2); /*0x1802fc211*/
      v12 = a1[6]; /*0x1802fc214*/
LABEL_11:
      v13 = (_DWORD *)(v11 + v12); /*0x1802fbfb1*/
      v55 = v13; /*0x1802fbfb4*/
      v14 = *(__int64 (__fastcall **)())(*(_QWORD *)v13 + 144LL); /*0x1802fbfbc*/
      if ( v14 == sub_1800A2BE0 ) /*0x1802fbfcd*/
        v15 = v13[6]; /*0x1802fbfd3*/
      else
        v15 = ((__int64 (__fastcall *)(_DWORD *))v14)(a1: v13); /*0x1802fc32a*/
      v16 = *a1; /*0x1802fbfd6*/
      v56 = v15; /*0x1802fbfe1*/
      v17 = *(__int64 **)(v16 + 8); /*0x1802fbfe5*/
      result = ((__int64 (__fastcall *)(_DWORD *, __int64, __int64 *, _DWORD **))sub_18007A740)( /*0x1802fbfec*/
                 a1: v13,
                 a2: v16,
                 a3: v17,
                 a4: &v55);
      v18 = a1[1]; /*0x1802fbff1*/
      if ( v18 == 0x7FFFFFFFFFFFFFELL ) /*0x1802fc006*/
        goto LABEL_65; /*0x1802fc006*/
      a1[1] = v18 + 1; /*0x1802fc010*/
      *(_QWORD *)(v16 + 8) = result; /*0x1802fc014*/
      *v17 = result; /*0x1802fc018*/
      goto LABEL_63; /*0x1802fc01b*/
    case 5: /*0x1802fbf31*/
      v43 = *((_DWORD *)a1 + 21); /*0x1802fc21d*/
      v44 = 0; /*0x1802fc221*/
      result = v43; /*0x1802fc223*/
      break; /*0x1802fc223*/
    default:
      return result;
  }
  while ( 1 ) /*0x1802fc234*/
  {
    v45 = 0; /*0x1802fc234*/
    if ( v43 < 32 ) /*0x1802fc239*/
      v45 = result; /*0x1802fc239*/
    v46 = 0; /*0x1802fc23d*/
    if ( v43 < 32 ) /*0x1802fc242*/
      v46 = v43; /*0x1802fc242*/
    v47 = v46; /*0x1802fc245*/
    v43 = v46 + 1; /*0x1802fc24f*/
    if ( (*(unsigned __int8 (__fastcall **)(__int64))(*(_QWORD *)(408 * v45 + a1[7]) + 128LL))(a1: a1[7] + 408LL * v46) == 0 ) /*0x1802fc268*/
      break; /*0x1802fc268*/
    ++v44; /*0x1802fc26a*/
    result = v45 + 1; /*0x1802fc26c*/
    if ( v44 >= 32 ) /*0x1802fc273*/
      return result; /*0x1802fc273*/
  }
  v48 = a1[7]; /*0x1802fc27a*/
  *((_DWORD *)a1 + 21) = v43; /*0x1802fc281*/
  (*(void (__fastcall **)(__int64, __int64))(*(_QWORD *)(408 * v47 + v48) + 8LL))(a1: 408 * v47 + v48, a2); /*0x1802fc291*/
  v49 = (_DWORD *)(408 * v47 + a1[7]); /*0x1802fc298*/
  v55 = v49; /*0x1802fc29b*/
  v50 = *(__int64 (__fastcall **)())(*(_QWORD *)v49 + 144LL); /*0x1802fc2a3*/
  if ( v50 == sub_1800A2BE0 ) /*0x1802fc2b4*/
    v51 = v49[6]; /*0x1802fc2b6*/
  else
    v51 = ((__int64 (__fastcall *)(_DWORD *))v50)(a1: v49); /*0x1802fc331*/
  v52 = *a1; /*0x1802fc2b9*/
  v56 = v51; /*0x1802fc2c4*/
  v53 = *(__int64 **)(v52 + 8); /*0x1802fc2c8*/
  v18 = ((__int64 (__fastcall *)(_DWORD *, __int64, __int64 *, _DWORD **))sub_18007A740)( /*0x1802fc2d4*/
          a1: v49,
          a2: v52,
          a3: v53,
          a4: &v55);
  v54 = a1[1]; /*0x1802fc2e1*/
  if ( v54 == 0x7FFFFFFFFFFFFFELL ) /*0x1802fc2ec*/
LABEL_65:
    sub_1800E2374(a1: "list<T> too long", a2: v18); /*0x1802fc31d*/
  result = v54 + 1; /*0x1802fc2ee*/
  a1[1] = result; /*0x1802fc2f1*/
  *(_QWORD *)(v52 + 8) = v18; /*0x1802fc2f5*/
  *v53 = v18; /*0x1802fc2f9*/
LABEL_63:
  *(_BYTE *)(a2 + 32) = 1; /*0x1802fc2fc*/
  return result; /*0x1802fc315*/
}
