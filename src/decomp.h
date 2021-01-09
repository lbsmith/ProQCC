
#ifndef _DECOMP_H
#define _DECOMP_H

void DecompileReadData(char *srcfile);
int DecompileGetFunctionIdxByName(char *name);
int DecompileAlreadySeen(char *fname);
void DecompileCalcProfiles(void);
char *DecompileGlobal(gofs_t ofs, def_t * req_t);
gofs_t DecompileScaleIndex(dfunction_t * df, gofs_t ofs);
char *DecompileImmediate(dfunction_t * df, gofs_t ofs, int fun, char *new);
char *DecompileGet(dfunction_t * df, gofs_t ofs, def_t * req_t);
void DecompileIndent(int c);
void DecompileDecompileStatement(dfunction_t * df, dstatement_t * s, int *indent);
void DecompileDecompileFunction(dfunction_t * df);
char *DecompileString(char *string);
char *DecompileValueString(etype_t type, void *val);
char *DecompilePrintParameter(ddef_t * def);
ddef_t *GetField(char *name);
ddef_t *DecompileGetParameter(gofs_t ofs);
void DecompileFunction(char *name);
void DecompileDecompileFunctions(void);
void DecompileProgsDat(char *name);
char *DecompileGlobalStringNoContents(gofs_t ofs);
char *DecompileGlobalString(gofs_t ofs);
void DecompilePrintStatement(dstatement_t * s);
void DecompilePrintFunction(char *name);

#endif
