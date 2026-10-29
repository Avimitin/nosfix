void __fastcall sub_1800D2630(unsigned int *a1)
{
  __int64 v2; // rsi
  __int64 v3; // r14
  __int64 *v4; // rdi
  unsigned int v5; // ebp
  __int64 v6; // rax
  __int64 v7; // rax
  int *v8; // rbx
  void (__fastcall *v9)(__int64 *, _QWORD, __int64, __int64); // r10
  __int64 v10; // r9
  int v11; // eax
  _BYTE v12[24]; // [rsp+30h] [rbp-58h] BYREF
  int v13; // [rsp+48h] [rbp-40h]
  int v14; // [rsp+4Ch] [rbp-3Ch]
  float v15[4]; // [rsp+50h] [rbp-38h] BYREF

  if ( a1 != nullptr ) /*0x1800d2633*/
  {
    v2 = *a1; /*0x1800d265b*/
    v3 = *((_QWORD *)a1 + 1); /*0x1800d2667*/
    v4 = *(__int64 **)(sub_1800D6F40() + 98304); /*0x1800d2670*/
    if ( (int)v2 > 0 ) /*0x1800d2679*/
    {
      (*(void (__fastcall **)(__int64 *, _QWORD, _QWORD))(*v4 + 520))(a1: v4, a2: 0, a3: 0); /*0x1800d2692*/
      (*(void (__fastcall **)(__int64 *, __int64))(*v4 + 712))(a1: v4, a2: 322); /*0x1800d26a3*/
      v5 = a1[1]; /*0x1800d26a9*/
      if ( v5 != 0 ) /*0x1800d26ae*/
      {
        v6 = sub_1800D6F40(); /*0x1800d26b0*/
        sub_1800D7E80(a1: v6, a2: v5); /*0x1800d26ba*/
        if ( a1[1] == 4 ) /*0x1800d26c3*/
        {
          (*(void (__fastcall **)(_QWORD, _QWORD, _BYTE *))(**(_QWORD **)(v3 + 152) + 136LL))( /*0x1800d26d6*/
            a1: *(_QWORD *)(v3 + 152),
            a2: 0,
            a3: v12);
          v7 = *v4; /*0x1800d2714*/
          v15[0] = 1.0 / (float)v13; /*0x1800d2717*/
          v15[1] = 1.0 / (float)v14; /*0x1800d2721*/
          (*(void (__fastcall **)(__int64 *, __int64, float *))(v7 + 872))(a1: v4, a2: 1, a3: v15); /*0x1800d2727*/
        }
      }
      v8 = (int *)(v3 + 144); /*0x1800d273d*/
      do /*0x1800d27f0*/
      {
        if ( *v8 < 0 ) /*0x1800d2748*/
        {
          v9 = *(void (__fastcall **)(__int64 *, _QWORD, __int64, __int64))(*v4 + 552); /*0x1800d275a*/
          if ( (*v8 & 0x10000000) != 0 ) /*0x1800d2765*/
          {
            v9(a1: v4, a2: 0, a3: 6, a4: 2); /*0x1800d276b*/
            v10 = 2; /*0x1800d276e*/
          }
          else
          {
            v9(a1: v4, a2: 0, a3: 6, a4: 1); /*0x1800d277c*/
            v10 = 1; /*0x1800d277f*/
          }
          (*(void (__fastcall **)(__int64 *, _QWORD, __int64, __int64))(*v4 + 552))(a1: v4, a2: 0, a3: 5, a4: v10); /*0x1800d2793*/
          v11 = *v8; /*0x1800d2799*/
          if ( (*v8 & 0x20000000) == 0 ) /*0x1800d279f*/
          {
            (*(void (__fastcall **)(__int64 *, _QWORD, _QWORD))(*v4 + 520))(a1: v4, a2: 0, a3: *((_QWORD *)v8 + 1)); /*0x1800d27ad*/
            (*(void (__fastcall **)(__int64 *, __int64, __int64, int *, int))(*v4 + 664))( /*0x1800d27d1*/
              a1: v4,
              a2: 5,
              a3: 2,
              a4: v8 - 36,
              a5: 24);
            v11 = *v8; /*0x1800d27d7*/
          }
          if ( (v11 & 0x40000000) == 0 ) /*0x1800d27dd*/
            *v8 = v11 & 0x7FFFFFFF; /*0x1800d27e3*/
        }
        v8 += 40; /*0x1800d27e5*/
        --v2; /*0x1800d27ec*/
      }
      while ( v2 != 0 ); /*0x1800d27f0*/
    }
  }
}
