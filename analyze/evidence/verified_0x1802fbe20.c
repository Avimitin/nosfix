__int64 __fastcall sub_1802FBE20(__int64 *a1)
{
  void **v2; // rcx
  void **v3; // rbx
  __int64 result; // rax
  _QWORD *i; // rbx
  unsigned __int8 *v6; // rcx
  __int64 (*v7)(void); // rdx

  v2 = *(void ***)qword_1813E46F8; /*0x1802fbe34*/
  *(_QWORD *)qword_1813E46F8 = qword_1813E46F8; /*0x1802fbe37*/
  *((_QWORD *)qword_1813E46F8 + 1) = qword_1813E46F8; /*0x1802fbe41*/
  qword_1813E4700 = 0; /*0x1802fbe4c*/
  if ( v2 != qword_1813E46F8 ) /*0x1802fbe57*/
  {
    do /*0x1802fbe77*/
    {
      v3 = (void **)*v2; /*0x1802fbe60*/
      j_j_free(Block: v2); /*0x1802fbe68*/
      v2 = v3; /*0x1802fbe74*/
    }
    while ( v3 != qword_1813E46F8 ); /*0x1802fbe77*/
  }
  result = *a1; /*0x1802fbe79*/
  for ( i = *(_QWORD **)*a1; i != (_QWORD *)*a1; i = (_QWORD *)*i ) /*0x1802fbe82*/
  {
    v6 = (unsigned __int8 *)i[2]; /*0x1802fbe90*/
    v7 = *(__int64 (**)(void))(*(_QWORD *)v6 + 184LL); /*0x1802fbe97*/
    if ( v7 == sub_1800A2C20 ) /*0x1802fbea1*/
      result = v6[68]; /*0x1802fbea3*/
    else
      result = v7(); /*0x1802fbecd*/
    if ( (_BYTE)result == 0 ) /*0x1802fbea9*/
      result = (*(__int64 (__fastcall **)(_QWORD))(*(_QWORD *)i[2] + 24LL))(a1: i[2]); /*0x1802fbeb2*/
  }
  return result; /*0x1802fbec2*/
}
