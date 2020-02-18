#include "os.h"
#include "cx.h"
#include "crypto.h"
#include "poseidon.h"

// E1/Fp : y^2 = x^3 + 7
// BN382_p =
// 5543634365110765627805495722742127385843376434033820803590214255538854698464778703795540858859767700241957783601153
// BN382_q =
// 5543634365110765627805495722742127385843376434033820803592568747918351978899288491582778380528407187068941959692289
// 382 bits = 48 bytes
// field modulus and group order differ only in the 25th - 32nd bytes (the start
// of the third row)
static const field field_modulus = {
    0x24, 0x04, 0x89, 0x3f, 0xda, 0xd8, 0x87, 0x8e, 0x71, 0x50, 0x3c, 0x69,
    0xb0, 0x9d, 0xbf, 0x88, 0xb4, 0x8a, 0x36, 0x14, 0x28, 0x9b, 0x09, 0x01,
    0x20, 0x12, 0x24, 0x6d, 0x22, 0x42, 0x41, 0x20, 0x00, 0x00, 0x00, 0x01,
    0x80, 0x0c, 0x18, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01};

static const scalar group_order = {
    0x24, 0x04, 0x89, 0x3f, 0xda, 0xd8, 0x87, 0x8e, 0x71, 0x50, 0x3c, 0x69,
    0xb0, 0x9d, 0xbf, 0x88, 0xb4, 0x8a, 0x36, 0x14, 0x28, 0x9b, 0x09, 0x01,
    0x80, 0x18, 0x30, 0x91, 0x83, 0x03, 0x01, 0x80, 0x00, 0x00, 0x00, 0x01,
    0x80, 0x0c, 0x18, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01};

// a = 0, b = 7
static const field group_coeff_b = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07};

static const field field_one = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01};

static const field field_two = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02};

static const field field_three = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03};

static const field field_four = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04};

static const field field_eight = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08};

static const field field_zero = {0};
static const scalar scalar_zero = {0};
static const affine affine_zero = {{0}, {0}};

// (X : Y : Z) = (0 : 1 : 0)
static const group group_zero = {
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01},
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};

// g_generator = (1 :
// 1587713460471950740217388326193312024737041813752165827005856534245539019723616944862168333942330219466268138558982
// : 1)
static const group group_one = {
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01},
    {0x0a, 0x50, 0xca, 0x03, 0xe4, 0xff, 0xad, 0x6e, 0x34, 0xfe, 0x4c, 0x72,
     0xf1, 0x3f, 0x2f, 0xbe, 0x5b, 0x32, 0xd0, 0x95, 0x41, 0xfc, 0x19, 0x5a,
     0x61, 0x91, 0x61, 0x76, 0x5f, 0x55, 0xc5, 0xce, 0x98, 0x43, 0xbe, 0x34,
     0x35, 0x3b, 0x8a, 0x3e, 0xfd, 0xc4, 0x03, 0xcd, 0x9d, 0x3c, 0x56, 0x06},
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01}};

static const affine affine_one = {
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01},
    {0x0a, 0x50, 0xca, 0x03, 0xe4, 0xff, 0xad, 0x6e, 0x34, 0xfe, 0x4c, 0x72,
     0xf1, 0x3f, 0x2f, 0xbe, 0x5b, 0x32, 0xd0, 0x95, 0x41, 0xfc, 0x19, 0x5a,
     0x61, 0x91, 0x61, 0x76, 0x5f, 0x55, 0xc5, 0xce, 0x98, 0x43, 0xbe, 0x34,
     0x35, 0x3b, 0x8a, 0x3e, 0xfd, 0xc4, 0x03, 0xcd, 0x9d, 0x3c, 0x56, 0x06}};

void field_add(field c, const field a, const field b) {
  cx_math_addm(c, a, b, field_modulus, field_bytes);
}

void field_sub(field c, const field a, const field b) {
  cx_math_subm(c, a, b, field_modulus, field_bytes);
}

void field_mul(field c, const field a, const field b) {
  cx_math_multm(c, a, b, field_modulus, field_bytes);
}

void field_sq(field c, const field a) {
  cx_math_multm(c, a, a, field_modulus, field_bytes);
}

void field_inv(field c, const field a) {
  cx_math_invprimem(c, a, field_modulus, field_bytes);
}

void field_negate(field c, const field a) {
  cx_math_subm(c, field_modulus, a, field_modulus, field_bytes);
}

// c = a^e mod m
// cx_math_powm(result_pointer, a, e, len_e, m, len(result)  (which is also
// len(a) and len(m)) )
void field_pow(field c, const field a, const field e) {
  cx_math_powm(c, a, e, 1, field_modulus, field_bytes);
}

unsigned int field_eq(const field a, const field b) {
  return (os_memcmp(a, b, field_bytes) == 0);
}

void scalar_add(scalar c, const scalar a, const scalar b) {
  cx_math_addm(c, a, b, group_order, scalar_bytes);
}

void scalar_sub(scalar c, const scalar a, const scalar b) {
  cx_math_subm(c, a, b, group_order, scalar_bytes);
}

void scalar_mul(scalar c, const scalar a, const scalar b) {
  cx_math_multm(c, a, b, group_order, scalar_bytes);
}

void scalar_sq(scalar c, const scalar a) {
  cx_math_multm(c, a, a, group_order, scalar_bytes);
}

// c = a^e mod m
// cx_math_powm(result_pointer, a, e, len_e, m, len(result)  (which is also
// len(a) and len(m)) )
void scalar_pow(scalar c, const scalar a, const scalar e) {
  cx_math_powm(c, a, e, 1, group_order, scalar_bytes);
}

unsigned int scalar_eq(const scalar a, const scalar b) {
  return (os_memcmp(a, b, scalar_bytes) == 0);
}

// zero is the only point with Z = 0 in jacobian coordinates
unsigned int is_zero(const group *p) {
  return (os_memcmp(p->Z, field_zero, field_bytes) == 0);
}

unsigned int affine_is_zero(const affine *p) {
  return (field_eq(p->x, field_zero) && field_eq(p->y, field_zero));
}

unsigned int is_on_curve(const group *p) {
  if (is_zero(p)) {
    return 1;
  }
  field lhs, rhs;

  if (field_eq(p->Z, field_one)) {
    // we can check y^2 == x^3 + ax + b
    field_sq(lhs, p->Y);                // y^2
    field_sq(rhs, p->X);                // x^2
    field_mul(rhs, rhs, p->X);          // x^3
    field_add(rhs, rhs, group_coeff_b); // x^3 + b
  } else {
    // we check (y/z)^2 == (x/z)^3 + b
    // => z(y^2 - bz^2) == x^3
    field x2, y2, z2;
    field_sq(x2, p->X);
    field_sq(y2, p->Y);
    field_sq(z2, p->Z);

    field_mul(lhs, z2, group_coeff_b); // bz^2
    field_sub(lhs, y2, lhs);           // y^2 - bz^2
    field_mul(lhs, p->Z, lhs);         // z(y^2 - bz^2)
    field_mul(rhs, p->X, x2);          // x^3
  }
  return (os_memcmp(lhs, rhs, field_bytes) == 0);
}

void affine_to_projective(group *r, const affine *p) {
  os_memcpy(r->X, p->x, field_bytes);
  os_memcpy(r->Y, p->y, field_bytes);
  os_memcpy(r->Z, field_one, field_bytes);
  return;
}

void projective_to_affine(affine *r, const group *p) {
  field zi, zi2, zi3;
  field_inv(zi, p->Z);        // 1/Z
  field_mul(zi2, zi, zi);     // 1/Z^2
  field_mul(zi3, zi2, zi);    // 1/Z^3
  field_mul(r->x, p->X, zi2); // X/Z^2
  field_mul(r->y, p->Y, zi3); // Y/Z^3
  return;
}


// https://www.hyperelliptic.org/EFD/g1p/auto-code/shortw/jacobian-0/doubling/dbl-2009-l.op3
// cost 2M + 5S + 6add + 3*2 + 1*3 + 1*8
// TODO: do we have complete formulas? :s
// TODO: get rid of some temp variables?
void group_dbl(group *r, const group *p) {
  if (is_zero(p)) {
    *r = *p;
  }

  field a, b, c;
  field_sq(a, p->X); // a = X1^2
  field_sq(b, p->Y); // b = Y1^2
  field_sq(c, b);    // c = b^2

  field t0, t1, t2, t3, d, e, f;
  field_add(t0, p->X, b);       // t0 = X1 + b
  field_sq(t1, t0);             // t1 = t0^2
  field_sub(t2, t1, a);         // t2 = t1 - a
  field_sub(t3, t2, c);         // t3 = t2 - c
  field_mul(d, field_two, t3);  // d = 2 * t3 TODO : * 2 = dbl
  field_mul(e, field_three, a); // e = 3 * a
  field_sq(f, e);               // f = e^2

  field t4;
  field_mul(t4, field_two, d); // t4 = 2 * d TODO : * 2 = dbl
  field_sub(r->X, f, t4);      // X = f - t4

  field t5, t6, t7;
  field_sub(t5, d, r->X);        // t5 = d - X
  field_mul(t6, field_eight, c); // t6 = 8 * c TODO : * 8 = dbl dbl dbl
  field_mul(t7, e, t5);          // t7 = e * t5
  field_sub(r->Y, t7, t6);       // Y = t7 - t6

  field t8;
  field_mul(t8, p->Y, p->Z);      // t8 = Y1 * Z1
  field_mul(r->Z, field_two, t8); // Z = 2 * t8 TODO * 2 : dbl
}

// https://www.hyperelliptic.org/EFD/g1p/auto-code/shortw/jacobian-0/addition/add-2007-bl.op3
// cost 11M + 5S + 9add + 4*2
// TODO: do we have complete formulas? :s
// TODO: get rid of some temp variables?
void group_add(group *r, const group *p, const group *q) {
  field z1z1, z2z2;
  field_sq(z1z1, p->Z); // Z1Z1 = Z1^2
  field_sq(z2z2, q->Z); // Z2Z2 = Z2^2

  field u1, u2, t0, s1, t1, s2;
  field_mul(u1, p->X, z2z2); // u1 = x1 * z2z2
  field_mul(u2, q->X, z1z1); // u2 = x2 * z1z1
  field_mul(t0, q->Z, z2z2); // t0 = z2 * z2z2
  field_mul(s1, p->Y, t0);   // s1 = y1 * t0
  field_mul(t1, p->Z, z1z1); // t1 = z1 * z1z1
  field_mul(s2, q->Y, t1);   // s2 = y2 * t1

  field h, t2, i, j, t3, w, v;
  field_sub(h, u2, u1);        // h = u2 - u1
  field_mul(t2, field_two, h); // t2 = 2 * h
  field_sq(i, t2);             // i = t2^2
  field_mul(j, h, i);          // j = h * i
  field_sub(t3, s2, s1);       // t3 = s2 - s1
  field_mul(w, field_two, t3); // w = 2 * t3
  field_mul(v, u1, i);         // v = u1 * i

  // X3 = w^2 - j - 2*v
  field t4, t5, t6;
  field_sq(t4, w);             // t4 = w^2
  field_mul(t5, field_two, v); // t5 = 2 * v
  field_sub(t6, t4, j);        // t6 = t4 - j
  field_sub(r->X, t6, t5);     // t6 - t5

  // Y3 = w * (v - X3) - 2*s1*j
  field t7, t8, t9, t10;
  field_sub(t7, v, r->X);       // t7 = v - X3
  field_mul(t8, s1, j);         // t8 = s1 * j
  field_mul(t9, field_two, t8); // t9 = 2 * t8
  field_mul(t10, w, t7);        // t10 = w * t7
  field_sub(r->Y, t10, t9);     // w * (v - X3) - 2*s1*j

  // Z3 = ((Z1 + Z2)^2 - Z1Z1 - Z2Z2) * h
  field t11, t12, t13, t14;
  field_add(t11, p->Z, q->Z); // t11 = z1 + z2
  field_sq(t12, t11);         // t12 = (z1 + z2)^2
  field_sub(t13, t12, z1z1);  // t13 = (z1 + z2)^2 - z1z1
  field_sub(t14, t13, z2z2);  // t14 = (z1 + z2)^2 - z1z1 - z2z2
  field_mul(r->Z, t14, h);    // ((z1 + z2)^2 - z1z1 - z2z2) * h
}

// https://www.hyperelliptic.org/EFD/g1p/auto-code/shortw/jacobian-0/addition/madd-2007-bl.op3
// for p = (X1, Y1, Z1), q = (X2, Y2, Z2); assumes Z2 = 1
// cost 7M + 4S + 9add + 3*2 + 1*4 ?
// TODO: get rid of some temp variables?
void group_madd(group *r, const group *p, const group *q) {

  if (is_zero(p)) {
    *r = *q;
    return;
  }
  if (is_zero(q)) {
    *r = *p;
    return;
  }

  field z1z1, u2;
  field_sq(z1z1, p->Z);      // z1z1 = Z1^2
  field_mul(u2, q->X, z1z1); // u2 = X2 * z1z1

  field t0, s2;
  field_mul(t0, p->Z, z1z1); // t0 = Z1 * z1z1
  field_mul(s2, q->Y, t0);   // s2 = Y2 * t0

  field h, hh;
  field_sub(h, u2, p->X); // h = u2 - X1
  field_sq(hh, h);        // hh = h^2

  field i, j, t1, w, v;
  field_mul(i, field_four, hh); // i = 4 * hh
  field_mul(j, h, i);           // j = h * i
  field_sub(t1, s2, p->Y);      // t1 = s2 - Y1
  field_mul(w, field_two, t1);  // w = 2 * t1
  field_mul(v, p->X, i);        // v = X1 * i

  // X3 = w^2 - J - 2*V
  field t2, t3, t4;
  field_sq(t2, w);             // t2 = w^2
  field_mul(t3, field_two, v); // t3 = 2*v
  field_sub(t4, t2, j);        // t4 = t2 - j
  field_sub(r->X, t4, t3);     // w^2 - j - 2*v

  // Y3 = w * (V - X3) - 2*Y1*J
  field t5, t6, t7, t8;
  field_sub(t5, v, r->X);       // t5 = v - X3
  field_mul(t6, p->Y, j);       // t6 = Y1 * j
  field_mul(t7, field_two, t6); // t7 = 2 * t6
  field_mul(t8, w, t5);         // t8 = w * t5
  field_sub(r->Y, t8, t7);      // w * (v - X3) - 2*Y1*j

  // Z3 = (Z1 + H)^2 - Z1Z1 - HH
  field t9, t10, t11;
  field_add(t9, p->Z, h);    // t9 = Z1 + h
  field_sq(t10, t9);         // t10 = t9^2
  field_sub(t11, t10, z1z1); // t11 = t10 - z1z1
  field_sub(r->Z, t11, hh);  // (Z1 + h)^2 - Z1Z1 - hh
}

void affine_scalar_mul(affine *r, const scalar k, const affine *p) {

  *r = affine_zero;
  if (affine_is_zero(p)) {
    return;
  }
  if (scalar_eq(k, scalar_zero)) {
    return;
  }

  group *pp, *pr;
  affine_to_projective(pp, p);
  affine_to_projective(pr, r);

  for (unsigned int i = scalar_offset; i < scalar_bits; i++) {
    unsigned int di = k[i / 8] & (1 << (7 - (i % 8)));
    group q0;
    group_dbl(&q0, pr);
    *pr = q0;
    if (di != 0) {
      group_add(&q0, pr, pp);
      *pr = q0;
    }
  }
  projective_to_affine(r, pr);
  return;
}

// Ledger uses:
// - BIP 39 to generate and interpret the master seed, which
//   produces the 24 words shown on the device at startup.
// - BIP 32 for HD key derivation (using the child key derivation function)
// - BIP 44 for HD account derivation (so e.g. btc and coda keys don't clash)

void generate_keypair(unsigned int index, affine *pub_key, scalar priv_key) {

  unsigned int bip32_path[5];
  unsigned char chain[32];

  bip32_path[0] = 44 | 0x80000000;
  bip32_path[1] = 49370 | 0x80000000;
  bip32_path[2] = index | 0x80000000;
  bip32_path[3] = 0;
  bip32_path[4] = 0;

  os_perso_derive_node_bip32(CX_CURVE_256K1, bip32_path,
                             sizeof(bip32_path) / sizeof(bip32_path[0]),
                             priv_key, chain);
  os_memcpy(priv_key + 32, chain, 32);
  os_memcpy(priv_key + 64, chain, 32);

  affine_scalar_mul(pub_key, priv_key, &group_one);
  // os_memset(priv_key, 0, sizeof(priv_key));
  return;
}

void generate_pubkey(affine *pub_key, const scalar priv_key) {
  affine_scalar_mul(pub_key, priv_key, &affine_one);
  return;
}

inline unsigned int is_odd(const field y) { return (y[field_bytes - 1] & 1); }

void poseidon_4in(scalar out, const scalar in1, const scalar in2,
                  const scalar in3, const scalar in4) {
  state pos = {{0}, {0}, {0}};
  scalar tmp[sponge_size - 1];

  os_memcpy(tmp[0], in1, scalar_bytes);
  os_memcpy(tmp[1], in2, scalar_bytes);
  poseidon(pos, tmp);
  os_memcpy(tmp[0], in3, scalar_bytes);
  os_memcpy(tmp[1], in4, scalar_bytes);
  poseidon(pos, tmp);
  poseidon_digest(pos, out);
  return;
}

void sign(field rx, scalar s, const affine *public_key,
          const scalar private_key, const scalar msg) {
  scalar k_prime;
  /* rx is G_io_apdu_buffer so we can take group_bytes bytes from it */
  {
    affine *r;
    r = (affine *)rx;
    poseidon_4in(k_prime, msg, public_key->x, public_key->y,
                 private_key);                  // k = hash(m || pk || sk)
    affine_scalar_mul(r, k_prime, &affine_one); // r = k*g

    /* store so we don't need group *r anymore */
    os_memcpy(rx, r->x, field_bytes);
    if (is_odd(r->y)) {
      field_negate(k_prime, k_prime); // if ry is odd, k = - k'
    }
  }
  poseidon_4in(s, msg, rx, public_key->x,
               public_key->y);   // e = hash(xr || pk || m)
  scalar_mul(s, s, private_key); // e*sk
  scalar_add(s, k_prime, s);     // k + e*sk
  return;
}
