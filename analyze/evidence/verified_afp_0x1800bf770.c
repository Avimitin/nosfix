__int64 __fastcall sub_1800BF770(__int64 a1)
{
  int *v1; // r9

  if ( (*(_BYTE *)(a1 + 32) & 4) != 0 ) /*0x1800bf77f*/
    return sub_1800BF862(); /*0x1800bf77f*/
  v1 = *(int **)(a1 + 336); /*0x1800bf785*/
  if ( v1 == nullptr || *v1 <= 0LL ) /*0x1800bf79d*/
    return sub_1800BF862(); /*0x1800bf77f*/
  else
    return sub_1800BF7A3(a1: 0); /*0x1800bf79e*/
}
