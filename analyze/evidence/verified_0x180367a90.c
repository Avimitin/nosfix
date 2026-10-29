__int64 sub_180367A90()
{
  _QWORD *v1; // [rsp+30h] [rbp+8h]

  if ( qword_1812D1E70 == 0 ) /*0x180367a9c*/
  {
    v1 = operator new(Size: 0x40u); /*0x180367aa8*/
    qword_1812D1E70 = (__int64)v1; /*0x180367ab4*/
    *v1 = &CDeviceDriver::`vftable'; /*0x180367abb*/
    v1[1] = 0; /*0x180367ac0*/
    v1[2] = 0; /*0x180367ac4*/
    v1[3] = 0; /*0x180367ac8*/
    v1[4] = 0; /*0x180367acc*/
    v1[5] = 0; /*0x180367ad0*/
    v1[6] = 0; /*0x180367ad4*/
    *((_DWORD *)v1 + 14) = 0; /*0x180367ad8*/
    *((_WORD *)v1 + 30) = 0; /*0x180367adb*/
  }
  return (unsigned int)(*(_DWORD *)(qword_1812D1E68 + 2084) - *(_DWORD *)(qword_1812D1E68 + 2080)); /*0x180367af4*/
}
