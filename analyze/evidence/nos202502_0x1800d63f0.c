__int64 __fastcall sub_1800D63F0(__int64 a1)
{
  __int64 v1; // rax
  CLibXact *v2; // rax
  __int64 v3; // rax
  __int64 v4; // rax

  afp_core_15(a1, a2: 3); /*0x1800d63fb*/
  v1 = sub_1800D6D00(); /*0x1800d6401*/
  sub_1800D6C70(a1: v1); /*0x1800d6409*/
  v2 = (CLibXact *)sub_1800D6F30(); /*0x1800d640e*/
  CLibXact::~CLibXact(this: v2); /*0x1800d6416*/
  v3 = sub_1800D8870(); /*0x1800d641b*/
  sub_1800D8810(a1: v3); /*0x1800d6423*/
  v4 = sub_1800DA910(); /*0x1800d6428*/
  return sub_1800D8940(a1: v4); /*0x1800d6430*/
}
