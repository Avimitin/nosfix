__int64 __fastcall sub_1800D2440(float *a1)
{
  return (unsigned __int8)(int)(float)(a1[2] * 255.0) /*0x1800d249d*/
       | (((unsigned __int8)(int)(float)(a1[1] * 255.0)
         | ((((unsigned __int8)(int)(float)(a1[3] * 255.0) << 8)
           | (unsigned int)(unsigned __int8)(int)(float)(*a1 * 255.0)) << 8)) << 8);
}
