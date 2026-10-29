__int128 *__fastcall sub_180304F30(int a1, __int64 a2, char a3, unsigned __int8 a4, unsigned int a5, int a6)
{
  __int128 *result; // rax
  int *v11; // r9
  unsigned __int64 v12; // r8
  int v13; // esi
  int v14; // r15d
  __int64 v15; // rcx
  __int128 *v16; // rax
  __int64 v17; // rdi
  __int64 v18; // rbx
  CLibXact *Instance; // rax
  __int64 v20; // rcx
  __int64 v21; // rdx
  int v22; // ebx
  __int64 v23; // rax
  float v24; // xmm7_4
  CLibXact *v25; // rax
  __int64 v26; // rcx
  __int128 v27; // [rsp+40h] [rbp-68h]
  _UNKNOWN *retaddr; // [rsp+A8h] [rbp+0h] BYREF

  result = (__int128 *)&retaddr; /*0x180304f30*/
  if ( (_BYTE)word_1813E472C != 0 ) /*0x180304f55*/
  {
    v11 = &dword_1813E56D8; /*0x180304f5f*/
    v12 = 0x180000000uLL; /*0x180304f6a*/
    v13 = 0; /*0x180304f75*/
    v14 = a6 + HIDWORD(qword_181366908); /*0x180304f82*/
    if ( a1 >= 0 ) /*0x180304f8c*/
    {
      v15 = 0; /*0x180304f92*/
      v16 = &xmmword_1813E4AD8; /*0x180304f94*/
      while ( *(_DWORD *)v16 != a1 ) /*0x180304fa2*/
      {
        v15 = (unsigned int)(v15 + 1); /*0x180304fa4*/
        v16 = (__int128 *)((char *)v16 + 24); /*0x180304fa6*/
        if ( (__int64)v16 >= (__int64)&dword_1813E56D8 ) /*0x180304fad*/
          goto LABEL_9; /*0x180304fad*/
      }
      if ( (_DWORD)v15 != -1 ) /*0x180304fb4*/
      {
        v17 = 3LL * (int)v15; /*0x180304fb9*/
        v18 = qword_1813E4AE8[3 * (int)v15]; /*0x180304fbd*/
        sub_180360060(a1: v15, a2, a3: 0x180000000uLL, a4: &dword_1813E56D8); /*0x180304fc5*/
        Instance = (CLibXact *)CLibXact::getInstance(); /*0x180304fca*/
        CLibXact::FadeOutSound(this: Instance, a2: v18, a3: false); /*0x180304fd9*/
        v12 = 0x180000000uLL; /*0x180304fdf*/
        *((_DWORD *)&xmmword_1813E4AD8 + 2 * v17) = -1; /*0x180304fe6*/
        v11 = &dword_1813E56D8; /*0x180304ff2*/
        *((_WORD *)&xmmword_1813E4AD8 + 4 * v17 + 2) = 0; /*0x180304ff9*/
        *((_BYTE *)&xmmword_1813E4AD8 + 8 * v17 + 6) = 0; /*0x180305002*/
        *((_QWORD *)&xmmword_1813E4AD8 + v17 + 1) = -1; /*0x18030500a*/
        qword_1813E4AE8[v17] = 0; /*0x180305016*/
      }
    }
LABEL_9:
    v20 = a5; /*0x18030501e*/
    v21 = (unsigned int)dword_1813E56D8; /*0x180305025*/
    *((_QWORD *)&v27 + 1) = -1; /*0x18030502b*/
    LODWORD(v27) = a1; /*0x180305034*/
    BYTE4(v27) = 0; /*0x180305038*/
    BYTE5(v27) = a3; /*0x180305042*/
    BYTE6(v27) = a4; /*0x180305047*/
    if ( a5 != -1 ) /*0x18030504f*/
      HIDWORD(v27) = a5 + dword_1813E56D8 + v14; /*0x180305057*/
    if ( v14 > 0 ) /*0x18030505e*/
      DWORD2(v27) = dword_1813E56D8 + v14; /*0x180305064*/
    v22 = 0; /*0x180305068*/
    result = &xmmword_1813E4AD8; /*0x18030506a*/
    while ( *(_DWORD *)result != -1 ) /*0x180305074*/
    {
      ++v22; /*0x180305076*/
      result = (__int128 *)((char *)result + 24); /*0x180305078*/
      if ( (__int64)result >= (__int64)&dword_1813E56D8 ) /*0x18030507f*/
        return result; /*0x18030507f*/
    }
    if ( v22 != -1 ) /*0x180305089*/
    {
      if ( v14 <= 0 ) /*0x180305092*/
      {
        if ( (unsigned __int8)(a3 - 1) <= 0x57u ) /*0x1803050a8*/
        {
          v23 = 0; /*0x1803050aa*/
          while ( *((_BYTE *)&xmmword_1813E4AD8 + v23 * 8 + 5) != a3 /*0x1803050cd*/
               || *(_DWORD *)((char *)&xmmword_1813E4AD8 + v23 * 8 + 8) != -1
               || qword_1813E4AE8[v23] != a2 )
          {
            ++v13; /*0x1803050cf*/
            v23 += 3; /*0x1803050d1*/
            if ( v23 >= 384 ) /*0x1803050db*/
              goto LABEL_30; /*0x1803050db*/
          }
          if ( v13 != -1 ) /*0x1803050e2*/
          {
            v21 = *((unsigned int *)&xmmword_1813E4AD8 + 6 * v13); /*0x1803050eb*/
            if ( (_DWORD)v21 != a1 && (int)v21 >= 0 ) /*0x1803050f9*/
              sub_1802BCAD0(a1: &xmmword_1813E4AD8); /*0x180305102*/
          }
        }
LABEL_30:
        sub_180360060(a1: v20, a2: v21, a3: v12, a4: v11); /*0x180305107*/
        v24 = *(float *)&dword_180654030[byte_1813E56DC]; /*0x18030511b*/
        v25 = (CLibXact *)CLibXact::getInstance(); /*0x18030512f*/
        CLibXact::SetSoundWithParam(this: v25, a2, a3: (float)a4, a4: v24, a5: -20000.0, a6: 0.0, a7: false); /*0x18030515d*/
      }
      v26 = 3LL * v22; /*0x180305184*/
      *(__int128 *)((char *)&xmmword_1813E4AD8 + 8 * v26) = v27; /*0x180305188*/
      qword_1813E4AE8[v26] = a2; /*0x180305190*/
      return (__int128 *)v22; /*0x18030517b*/
    }
  }
  return result; /*0x1803051b9*/
}
