#include "data.table.h"

void which_int(int *x, int nx, int *out, int *nout, int val) {
  int *buf = malloc(nx*sizeof(buf));
  if (!buf) error("%s: Unable to allocate memory", __func__); // # nocov
  int j=0;
  for (int i=0; i<nx; i++) {
    if (x[i] == val) {
      buf[j] = i;
      j++;
    }
  }
  nout[0] = j+1;
  memcpy(out, buf, j*sizeof(buf));
}
void which_double(double *x, int nx, int *out, int *nout, double val) {
  int *buf = malloc(nx*sizeof(buf));
  if (!buf) error("%s: Unable to allocate memory", __func__); // # nocov
  int j=0;
  if (ISNA(val)) {
    for (int i=0; i<nx; i++) {
      if (ISNA(x[i])) {
        buf[j] = i;
        j++;
      }
    }
  } else {
    for (int i=0; i<nx; i++) {
      if (x[i] == val) {
        buf[j] = i;
        j++;
      }
    }
  }
  nout[0] = j+1;
  memcpy(out, buf, j*sizeof(buf));
}
void which_char(SEXP x, int nx, int *out, int *nout, SEXP val) {
  int *buf = malloc(nx*sizeof(buf));
  if (!buf) error("%s: Unable to allocate memory", __func__); // # nocov
  int j=0;
  if (STRING_ELT(val, 0) == NA_STRING) {
    for (int i=0; i<nx; i++) {
      if (STRING_ELT(x, i) == NA_STRING) {
        buf[j] = i;
        j++;
      }
    }
  } else {
    for (int i=0; i<nx; i++) {
      if (STRING_ELT(x, i) == STRING_ELT(val, 0)) {
        buf[j] = i;
        j++;
      }
    }
  }
  nout[0] = j+1;
  memcpy(out, buf, j*sizeof(buf));
}
/*SEXP whichna_charR(SEXP x) {
  int protecti = 0;
  R_len_t nx = length(x);
  SEXP ans = PROTECT(allocVector(INTSXP, nx)); protecti++;
  int *ians = INTEGER(ans);
  int nans=0;
  SEXP val = PROTECT(allocNAVector(STRSXP, 1)); protecti++;
  which_char(x, nx, ians, &nans, val);
  SETLENGTH(ans, nans-1);
  UNPROTECT(protecti);
  return ans;
}*/
SEXP coalesce(SEXP x, SEXP values, SEXP inplace) {
  int JJ = length(values), protecti = 0, nx = length(x);

  int *iwhich = malloc(nx*sizeof(iwhich));
  if (!iwhich) error("%s: Unable to allocate memory", __func__); // # nocov
  int nwhich=0;

  if (!isLogical(inplace) || length(inplace)!=1 || LOGICAL(inplace)[0]==NA_LOGICAL)
    error("internal error: inplace must be TRUE or FALSE"); // # nocov
  bool binplace = LOGICAL(inplace)[0];

  switch(TYPEOF(x)) {
  case LGLSXP: {
    which_int(LOGICAL(x), nx, iwhich, &nwhich, NA_LOGICAL);
  } break;
  case INTSXP: {
    which_int(INTEGER(x), nx, iwhich, &nwhich, NA_INTEGER);
  } break;
  case REALSXP: {
    which_double(REAL(x), nx, iwhich, &nwhich, inherits(x,"integer64") ? NA_INT64_D : NA_REAL);
  } break;
  case STRSXP: {
    SEXP val = PROTECT(allocNAVector(STRSXP, 1)); protecti++;
    which_char(x, nx, iwhich, &nwhich, val);
  } break;
  default: error("Incompatible type");
  }

  if (nwhich==0) return(binplace ? x : duplicate(x));

  int *valueslens = malloc(JJ*sizeof(valueslens));
  if (!valueslens) error("%s: Unable to allocate memory", __func__); // # nocov
  for (int j=0; j<JJ; j++) {
    valueslens[j] = LENGTH(VECTOR_ELT(values, j));
  }
  SEXP out = R_NilValue;
  if (binplace) out = x; else {
    out = PROTECT(duplicate(x)); protecti++;
  }
  switch(TYPEOF(x)) {
  case LGLSXP: {
    int *out_ptr = LOGICAL(out);
    for (int i=0; i<nwhich-1; i++) {
      int j = 0;
      while (j < JJ) {
        int this = LOGICAL(VECTOR_ELT(values, j))[valueslens[j]==1 ? 0 : iwhich[i]];
        if (this != NA_LOGICAL) {
          out_ptr[iwhich[i]] = this;
          break;
        }
        j++;
      }
    }
  } break;
  case INTSXP: {
    int *out_ptr = INTEGER(out);
    for (int i=0; i<nwhich-1; i++) {
      int j = 0;
      while (j < JJ) {
        int this = INTEGER(VECTOR_ELT(values, j))[valueslens[j]==1 ? 0 : iwhich[i]];
        if (this != NA_INTEGER) {
          out_ptr[iwhich[i]] = this;
          break;
        }
        j++;
      }
    }
  } break;
  case REALSXP: {
    double *out_ptr = REAL(out);
    if (!inherits(x,"integer64")) { // numeric
      for (int i=0; i<nwhich-1; i++) {
        int j = 0;
        while (j < JJ) {
          double this = REAL(VECTOR_ELT(values, j))[valueslens[j]==1 ? 0 : iwhich[i]];
          if (!ISNA(this)) {
            out_ptr[iwhich[i]] = this;
            break;
          }
          j++;
        }
      }
    } else { // integer64
      for (int i=0; i<nwhich-1; i++) {
        int j = 0;
        while (j < JJ) {
          double this = REAL(VECTOR_ELT(values, j))[valueslens[j]==1 ? 0 : iwhich[i]];
          if (this != NA_INT64_D) {
            out_ptr[iwhich[i]] = this;
            break;
          }
          j++;
        }
      }
    }
  } break;
  case STRSXP: {
    for (int i=0; i<nwhich-1; i++) {
      int j = 0;
      while (j < JJ) {
        SEXP this = STRING_ELT(VECTOR_ELT(values, j), valueslens[j]==1 ? 0 : iwhich[i]);
        if (this != NA_STRING) {
          SET_STRING_ELT(out, iwhich[i], this);
          break;
        }
        j++;
      }
    }
  } break;
  default: error("Unrecognized type.");
  }

  UNPROTECT(protecti);
  return out;
}
