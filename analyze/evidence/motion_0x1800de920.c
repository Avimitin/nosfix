// Filled ribbon primitive draw: payload count+0,buffer+8. DrawPrimitiveUP trianglelist count*2,vertexstride20 XYZRHW/color. 120 bytes per quad. Readonly source; nosfix owned geometry can be used until call returns.
void __fastcall sub_1800DE920(__int64 a1, __int64 a2, __int64 a3, __int64 a4)
{
  __int64 v4; // rsi
  int v5; // edi
  __int64 v6; // rbx
  __int64 v7; // rax

  if ( a4 != 0 ) /*0x1800de927*/
  {
    v4 = *(_QWORD *)(a4 + 8); /*0x1800de932*/
    v5 = *(_DWORD *)a4; /*0x1800de93b*/
    if ( *(int *)a4 > 0 ) /*0x1800de940*/
    {
      v6 = *(_QWORD *)(sub_1800D6F40() + 98304); /*0x1800de950*/
      v7 = sub_1800D6F40(); /*0x1800de957*/
      (*(void (__fastcall **)(__int64, _QWORD, _QWORD))(*(_QWORD *)v6 + 520LL))( /*0x1800de96b*/
        a1: v6,
        a2: 0,
        a3: *(_QWORD *)(v7 + 98440));
      (*(void (__fastcall **)(__int64, _QWORD, __int64, __int64))(*(_QWORD *)v6 + 552LL))(a1: v6, a2: 0, a3: 6, a4: 1); /*0x1800de982*/
      (*(void (__fastcall **)(__int64, _QWORD, __int64, __int64))(*(_QWORD *)v6 + 552LL))(a1: v6, a2: 0, a3: 5, a4: 1); /*0x1800de998*/
      (*(void (__fastcall **)(__int64, _QWORD, __int64, __int64))(*(_QWORD *)v6 + 552LL))(a1: v6, a2: 0, a3: 1, a4: 3); /*0x1800de9ae*/
      (*(void (__fastcall **)(__int64, _QWORD, __int64, __int64))(*(_QWORD *)v6 + 552LL))(a1: v6, a2: 0, a3: 2, a4: 3); /*0x1800de9c4*/
      (*(void (__fastcall **)(__int64, __int64))(*(_QWORD *)v6 + 712LL))(a1: v6, a2: 68); /*0x1800de9d5*/
      (*(void (__fastcall **)(__int64, __int64, _QWORD, __int64, int))(*(_QWORD *)v6 + 664LL))( /*0x1800de9f5*/
        a1: v6,
        a2: 4,
        a3: (unsigned int)(2 * v5),
        a4: v4,
        a5: 20);
    }
  }
}
