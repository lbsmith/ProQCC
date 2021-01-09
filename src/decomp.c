#include "screen.h"
#include "qcc.h"
#include "decomp.h"
//#include <unistd.h>
#include <stdio.h>

FILE *Decompileofile;
FILE *Decompileprogssrc;
FILE *Decompileprofile;
char *DecompileFilesSeen[1024];
int DecompileFileCtr = 0;
char *DecompileProfiles[MAX_FUNCTIONS];

char *type_names[8] =
{
    "void",
		"string",
		"float",
		"vector",
		"entity",
		"ev_field",
		"void()",
		"ev_pointer"
};

extern float pr_globals[MAX_REGS];
extern int numpr_globals;

extern char strings[MAX_STRINGS];
extern int strofs;

extern dstatement_t statements[MAX_STATEMENTS];
extern int numstatements;
extern int statement_linenums[MAX_STATEMENTS];

extern dfunction_t functions[MAX_FUNCTIONS];
extern int numfunctions;

extern ddef_t globals[MAX_GLOBALS];
extern int numglobaldefs;

extern ddef_t fields[MAX_FIELDS];
extern int numfielddefs;

int maindecstatus = 6;

static char *builtins[79] =
{
	
    NULL,
		"void (vector ang)",
		"void (entity e, vector o)",
		"void (entity e, string m)",
		"void (entity e, vector min, vector max)",
		NULL,
		"void ()",
		"float ()",
		"void (entity e, float chan, string samp, float vol, float atten)",
		"vector (vector v)",
		"void (string e)",
		"void (string e)",
		"float (vector v)",
		"float (vector v)",
		"entity ()",
		"void (entity e)",
		"void (vector v1, vector v2, float nomonsters, entity forent)",
		"entity ()",
		"entity (entity start, .string fld, string match)",
		"string (string s)",
		"string (string s)",
		"void (entity client, string s)",
		"entity (vector org, float rad)",
		"void (string s)",
		"void (entity client, string s)",
		"void (string s)",
		"string (float f)",
		"string (vector v)",
		"void ()",
		"void ()",
		"void ()",
		"void (entity e)",
		"float (float yaw, float dist)",
		NULL,
		"float (float yaw, float dist)",
		"void (float style, string value)",
		"float (float v)",
		"float (float v)",
		"float (float v)",
		NULL,
		"float (entity e)",
		"float (vector v)",
		NULL,
		"float (float f)",
		"vector (entity e, float speed)",
		"float (string s)",
		"void (string s)",
		"entity (entity e)",
		"void (vector o, vector d, float color, float count)",
		"void ()",
		NULL,
		"vector (vector v)",
		"void (float to, float f)",
		"void (float to, float f)",
		"void (float to, float f)",
		"void (float to, float f)",
		"void (float to, float f)",
		"void (float to, float f)",
		"void (float to, string s)",
		"void (float to, entity s)",
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		"void (float step)",
		"string (string s)",
		"void (entity e)",
		"void (string s)",
		NULL,
		"void (string var, string val)",
		"void (entity client, string s)",
		"void (vector pos, string samp, float vol, float atten)",
		"string (string s)",
		"string (string s)",
		"string (string s)",
		"void (entity e)"
		
};

char *DecompileValueString(etype_t type, void *val);
ddef_t *DecompileGetParameter(gofs_t ofs);
char *DecompilePrintParameter(ddef_t * def);

void 
DecompileReadData(char *srcfile)
{
    dprograms_t progs;
    int h;
	
    h = SafeOpenRead(srcfile);
    SafeRead(h, &progs, sizeof(progs));
	
    lseek(h, progs.ofs_strings, SEEK_SET);
    strofs = progs.numstrings;
    SafeRead(h, strings, strofs);
	
    lseek(h, progs.ofs_statements, SEEK_SET);
    numstatements = progs.numstatements;
    SafeRead(h, statements, numstatements * sizeof(dstatement_t));
	
    lseek(h, progs.ofs_functions, SEEK_SET);
    numfunctions = progs.numfunctions;
    SafeRead(h, functions, numfunctions * sizeof(dfunction_t));
	
    lseek(h, progs.ofs_globaldefs, SEEK_SET);
    numglobaldefs = progs.numglobaldefs;
    SafeRead(h, globals, numglobaldefs * sizeof(ddef_t));
	
    lseek(h, progs.ofs_fielddefs, SEEK_SET);
    numfielddefs = progs.numfielddefs;
    SafeRead(h, fields, numfielddefs * sizeof(ddef_t));
	
    lseek(h, progs.ofs_globals, SEEK_SET);
    numpr_globals = progs.numglobals;
    SafeRead(h, pr_globals, numpr_globals * 4);
	
    CPrintf("$f9");
    MoveCurs(50, maindecstatus);
    CPrintf("Read Data from %s:\n", srcfile);
    maindecstatus++;
    maindecstatus++;
    MoveCurs(50, maindecstatus);
    CPrintf("Total Size is %6i\n", (int)lseek(h, 0, SEEK_END));
    maindecstatus++;
    MoveCurs(50, maindecstatus);
    CPrintf("Version Code is %i\n", progs.version);
    maindecstatus++;
    MoveCurs(50, maindecstatus);
    CPrintf("CRC is %i\n", progs.crc);
    maindecstatus++;
    MoveCurs(50, maindecstatus);
    CPrintf("%6i strofs\n", strofs);
    maindecstatus++;
    MoveCurs(50, maindecstatus);
    CPrintf("%6i numstatements\n", numstatements);
    maindecstatus++;
    MoveCurs(50, maindecstatus);
    CPrintf("%6i numfunctions\n", numfunctions);
    maindecstatus++;
    MoveCurs(50, maindecstatus);
    CPrintf("%6i numglobaldefs\n", numglobaldefs);
    maindecstatus++;
    MoveCurs(50, maindecstatus);
    CPrintf("%6i numfielddefs\n", numfielddefs);
    maindecstatus++;
    MoveCurs(50, maindecstatus);
    CPrintf("%6i numpr_globals\n", numpr_globals);
    maindecstatus++;
    MoveCurs(50, maindecstatus);
	
}

int 
DecompileGetFunctionIdxByName(char *name)
{
	
    int i;
	
    for (i = 1; i < numfunctions; i++)
		if (!strcmp(name, strings + functions[i].s_name)) {
			break;
		}
		return i;
}

int 
DecompileAlreadySeen(char *fname)
{
	
    int i;
    char *new;
	
    if (DecompileFileCtr > 1000)
		Error("Fatal Error - too many source files.");
	
    for (i = 0; i < DecompileFileCtr; i++) {
		if (!strcmp(fname, DecompileFilesSeen[i]))
			return 1;
    }
	
    new = (char *)SafeMalloc(strlen(fname) + 1);
    strcpy(new, fname);
    DecompileFilesSeen[DecompileFileCtr] = new;
    DecompileFileCtr++;
	
    TimeUpdate();
	
    ShowTempEntry("$f3decompiling %s        \n", fname);
	
    return 0;
}

void 
DecompileCalcProfiles(void)
{
	
    int i, j, ps;
    char *new;
    static char fname[512];
    static char line[512];
    dfunction_t *df;
    dstatement_t *ds, *rds;
    ddef_t *par;
    unsigned short dom;
	
    for (i = 1; i < numfunctions; i++) {
		
		df = functions + i;
		fname[0] = '\0';
		line[0] = '\0';
		DecompileProfiles[i] = NULL;
		
		if (df->first_statement <= 0) {
			
			sprintf(fname, "%s %s", builtins[-df->first_statement], strings + functions[i].s_name);
			
		} else {
			
			ds = statements + df->first_statement;
			rds = NULL;
			
			/*
			* find a return statement, to determine the result type 
			*/
			
			while (1) {
				
				dom = (ds->op) % 100;
				if (!dom)
					break;
				if (dom == OP_RETURN) {
					rds = ds;
					/*		    break; */
				}
				ds++;
			}
			
			/*
			* print the return type  
			*/
			
			if ((rds != NULL) && (rds->a != 0)) {
				
				par = DecompileGetParameter(rds->a);
				
				if (par) {
					sprintf(fname, "%s ", type_names[par->type]);
				} else {
					sprintf(fname, "float /* Warning: Could not determine return type */ ");
				}
				
			} else {
				sprintf(fname, "\nvoid ");
			}
			strcat(fname, "(");
			
			/*
			* determine overall parameter size 
			*/
			
			for (j = 0, ps = 0; j < df->numparms; j++)
				ps += df->parm_size[j];
			
			if (ps > 0) {
				
				for (j = df->parm_start; j < (df->parm_start) + ps; j++) {
					
					line[0] = '\0';
					par = DecompileGetParameter(j);
					
					if (!par)
						Error("Error - No parameter names with offset %i.", j);
					
					if (par->type == ev_vector)
						j += 2;
					
					if (j < (df->parm_start) + ps - 1) {
						sprintf(line, "%s, ", DecompilePrintParameter(par));
					} else {
						sprintf(line, "%s", DecompilePrintParameter(par));
					}
					strcat(fname, line);
				}
				
			}
			strcat(fname, ") ");
			line[0] = '\0';
			sprintf(line, strings + functions[i].s_name);
			strcat(fname, line);
			
		}
		
		if (i >= MAX_FUNCTIONS)
			Error("Fatal Error - too many functions.");
		
		new = (char *)SafeMalloc(strlen(fname) + 1);
		strcpy(new, fname);
		DecompileProfiles[i] = new;
		
    }
	
}

char *
DecompileGlobal(gofs_t ofs, def_t * req_t)
{
    int i;
    ddef_t *def;
    static char line[256];
    char *res;
    char found = 0;
	
    line[0] = '\0';
	
    def = NULL;
	
    for (i = 0; i < numglobaldefs; i++) {
		def = &globals[i];
		
		if (def->ofs == ofs) {
		/*
		printf("DecompileGlobal - Found %i at %i.\n", ofs, (int)def);
			*/
			found = 1;
			break;
		}
    }
	
    if (found) {
		
		if (!strcmp(strings + def->s_name, "IMMEDIATE"))
			sprintf(line, "%s", DecompileValueString(def->type, &pr_globals[def->ofs]));
		else {
			
			sprintf(line, "%s", strings + def->s_name);
			if (def->type == ev_vector && req_t == &def_float)
				strcat(line, "_x");
			
		}
		res = (char *)SafeMalloc(strlen(line) + 1);
		strcpy(res, line);
		/*
		printf("DecompileGlobal - Found \"%s\"(%i) at %i.\n", line, ofs, (int)def);
		*/
		
		return res;
    }
    return NULL;
}

gofs_t 
DecompileScaleIndex(dfunction_t * df, gofs_t ofs)
{
    gofs_t nofs = 0;
	
    if (ofs > RESERVED_OFS)
		nofs = ofs - df->parm_start + RESERVED_OFS;
    else
		nofs = ofs;
	
    return nofs;
}

#define MAX_NO_LOCAL_IMMEDIATES 4096 /* dln: increased, not enough! */

char *
DecompileImmediate(dfunction_t * df, gofs_t ofs, int fun, char *new)
{
    int i;
    char *res;
    static char *IMMEDIATES[MAX_NO_LOCAL_IMMEDIATES];
    gofs_t nofs;
	
    /*
	* free 'em all 
	*/
    if (fun == 0) {
	/*
	printf("DecompileImmediate - Initializing function environment.\n");
		*/
		
		for (i = 0; i < MAX_NO_LOCAL_IMMEDIATES; i++)
			if (IMMEDIATES[i]) {
				free(IMMEDIATES[i]);
				IMMEDIATES[i] = NULL;
			}
			return NULL;
    }
    nofs = DecompileScaleIndex(df, ofs);
	/*
    printf("DecompileImmediate - Index scale: %i -> %i.\n", ofs, nofs);
	*/
	
    /*
	* check consistency 
	*/
    if ((nofs <= 0) || (nofs > MAX_NO_LOCAL_IMMEDIATES - 1))
		Error("Fatal Error - Index (%i) out of bounds.\n", nofs);
	
		/*
		* insert at nofs 
	*/
    if (fun == 1) {
		
		if (IMMEDIATES[nofs])
			free(IMMEDIATES[nofs]);
		
		IMMEDIATES[nofs] = (char *)SafeMalloc(strlen(new) + 1);
		strcpy(IMMEDIATES[nofs], new);
		/*
		printf("DecompileImmediate - Putting \"%s\" at index %i.\n", new, nofs);
		*/
    }
    /*
	* get from nofs 
	*/
    if (fun == 2) {
		
		if (IMMEDIATES[nofs]) {
		/*
		printf("DecompileImmediate - Reading \"%s\" at index %i.\n", IMMEDIATES[nofs], nofs);
			*/
			res = (char *)SafeMalloc(strlen(IMMEDIATES[nofs]) + 1);
			strcpy(res, IMMEDIATES[nofs]);
			
			return res;
		} else
			Error("Error - %i not defined.", nofs);
    }
    return NULL;
}

char *
DecompileGet(dfunction_t * df, gofs_t ofs, def_t * req_t)
{
    char *arg1 = NULL;
	
    arg1 = DecompileGlobal(ofs, req_t);
	
    if (arg1 == NULL)
		arg1 = DecompileImmediate(df, ofs, 2, NULL);
	
		/*
		if (arg1)
		printf("DecompileGet - found \"%s\".\n", arg1);
	*/
	
    return arg1;
}

void DecompilePrintStatement(dstatement_t * s);

void 
DecompileIndent(int c)
{
    int i;
	
    if (c < 0)
		c = 0;
	
    for (i = 0; i < c; i++) {
		
		fprintf(Decompileofile, "   ");
    }
}

void 
DecompileDecompileStatement(dfunction_t * df, dstatement_t * s, int *indent)
{
    static char line[512];
    static char fnam[512];
    char *arg1, *arg2, *arg3;
    int nargs, i, j;
    dstatement_t *t;
    unsigned short dom, doc, ifc, tom;
    def_t *typ1, *typ2, *typ3;
	
    dstatement_t *k;
    int dum;
	
    arg1 = arg2 = arg3 = NULL;
	
    line[0] = '\0';
    fnam[0] = '\0';
	
    dom = s->op;
	
    doc = dom / 10000;
    ifc = (dom % 10000) / 100;
	
    /*
	* use program flow information 
	*/
    for (i = 0; i < ifc; i++) {
		(*indent)--;
		fprintf(Decompileofile, "\n");
		DecompileIndent(*indent);
		fprintf(Decompileofile, "}\n");
    }
    for (i = 0; i < doc; i++) {
		DecompileIndent(*indent);
		fprintf(Decompileofile, "do {\n\n");
		(*indent)++;
    }
	
    /*
	* remove all program flow information 
	*/
    s->op %= 100;
	
    typ1 = pr_opcodes[s->op].type_a;
    typ2 = pr_opcodes[s->op].type_b;
    typ3 = pr_opcodes[s->op].type_c;
	
    /*
	* printf("DecompileDecompileStatement - decompiling %i (%i):\n",(int)(s - statements),dom);
	* DecompilePrintStatement (s);  
	*/
    /*
	* states are handled at top level 
	*/
	
    if (s->op == OP_DONE || s->op == OP_STATE) {
		
    } else if (s->op == OP_RETURN) {
		
		DecompileIndent(*indent);
		fprintf(Decompileofile, "return ");
		
		if (s->a) {
			arg1 = DecompileGet(df, s->a, typ1);
			fprintf(Decompileofile, "( %s )", arg1);
		}
		fprintf(Decompileofile, ";\n");
		
    } else if ((OP_MUL_F <= s->op && s->op <= OP_SUB_V) ||
		(OP_EQ_F <= s->op && s->op <= OP_GT) ||
		(OP_AND <= s->op && s->op <= OP_BITOR)) {
		
		arg1 = DecompileGet(df, s->a, typ1);
		arg2 = DecompileGet(df, s->b, typ2);
		arg3 = DecompileGlobal(s->c, typ3);
		
		if (arg3) {
			DecompileIndent(*indent);
			fprintf(Decompileofile, "%s = %s %s %s;\n", arg3, arg1, pr_opcodes[s->op].name, arg2);
		} else {
			sprintf(line, "(%s %s %s)", arg1, pr_opcodes[s->op].name, arg2);
			DecompileImmediate(df, s->c, 1, line);
		}
		
    } else if (OP_LOAD_F <= s->op && s->op <= OP_ADDRESS) {
		
		arg1 = DecompileGet(df, s->a, typ1);
		arg2 = DecompileGet(df, s->b, typ2);
		arg3 = DecompileGlobal(s->c, typ3);
		
		if (arg3) {
			DecompileIndent(*indent);
			fprintf(Decompileofile, "%s = %s.%s;\n", arg3, arg1, arg2);
		} else {
			sprintf(line, "%s.%s", arg1, arg2);
			DecompileImmediate(df, s->c, 1, line);
		}
    } else if (OP_STORE_F <= s->op && s->op <= OP_STORE_FNC) {
		
		arg1 = DecompileGet(df, s->a, typ1);
		arg3 = DecompileGlobal(s->b, typ2);
		
		if (arg3) {
			DecompileIndent(*indent);
			fprintf(Decompileofile, "%s = %s;\n", arg3, arg1);
		} else {
			sprintf(line, "%s", arg1);
			DecompileImmediate(df, s->b, 1, line);
		}
		
    } else if (OP_STOREP_F <= s->op && s->op <= OP_STOREP_FNC) {
		
		arg1 = DecompileGet(df, s->a, typ1);
		arg2 = DecompileGet(df, s->b, typ2);
		
		DecompileIndent(*indent);
		fprintf(Decompileofile, "%s = %s;\n", arg2, arg1);
		
    } else if (OP_NOT_F <= s->op && s->op <= OP_NOT_FNC) {
		
		arg1 = DecompileGet(df, s->a, typ1);
		sprintf(line, "!%s", arg1);
		DecompileImmediate(df, s->c, 1, line);
		
    } else if (OP_CALL0 <= s->op && s->op <= OP_CALL8) {
		
		nargs = s->op - OP_CALL0;
		
		arg1 = DecompileGet(df, s->a, NULL);
		sprintf(line, "%s (", arg1);
		sprintf(fnam, "%s", arg1);
		
		for (i = 0; i < nargs; i++) {
			
			typ1 = NULL;
			
			j = 4 + 3 * i;
			
			if (arg1)
				free(arg1);
			
			arg1 = DecompileGet(df, j, typ1);
			strcat(line, arg1);
			
#ifndef DONT_USE_DIRTY_TRICKS
			if (!strcmp(fnam, "WriteCoord"))
				if (!strcmp(arg1, "org") || !strcmp(arg1, "trace_endpos") || !strcmp(arg1, "p1") || !strcmp(arg1, "p2") || !strcmp(arg1, "o"))
					strcat(line, "_x");
#endif
				
				if (i < nargs - 1)
					strcat(line, ",");
		}
		
		strcat(line, ")");
		DecompileImmediate(df, 1, 1, line);
		
		/*
		* if ( ( ( (s+1)->a != 1) && ( (s+1)->b != 1) && 
		* ( (s+2)->a != 1) && ( (s+2)->b != 1) ) || 
		* ( ((s+1)->op) % 100 == OP_CALL0 ) ) {
		* DecompileIndent(*indent);
		* fprintf(Decompileofile,"%s;\n",line);
		* }
		*/
		
		if ((((s + 1)->a != 1) && ((s + 1)->b != 1) &&
			((s + 2)->a != 1) && ((s + 2)->b != 1)) ||
			((((s + 1)->op) % 100 == OP_CALL0) && ((((s + 2)->a != 1)) || ((s + 2)->b != 1)))) {
			DecompileIndent(*indent);
			fprintf(Decompileofile, "%s;\n", line);
		}
    } else if (s->op == OP_IF || s->op == OP_IFNOT) {
		
		arg1 = DecompileGet(df, s->a, NULL);
		arg2 = DecompileGlobal(s->a, NULL);
		
		if (s->op == OP_IFNOT) {
			
			if (s->b < 1)
				Error("Found a negative IFNOT jump.");
			
				/*
				* get instruction right before the target 
			*/
			t = s + s->b - 1;
			tom = t->op % 100;
			
			if (tom != OP_GOTO) {
				
			/*
			* pure if 
				*/
				DecompileIndent(*indent);
#if 0
				fprintf(Decompileofile, "if ( %s ) { /*1*/\n\n", arg1);
#endif
				fprintf(Decompileofile, "if ( %s ) {\n\n", arg1);
				(*indent)++;
				
			} else {
				
				if (t->a > 0) {
				/*
				* ite 
					*/
					
					DecompileIndent(*indent);
#if 0
					fprintf(Decompileofile, "if ( %s ) { /*2*/\n\n", arg1);
#endif
					fprintf(Decompileofile, "if ( %s ) {\n\n", arg1);
					(*indent)++;
					
				} else {
					
#if 0
					if ((((t->a + s->b) == 0) ||
						((arg2 != NULL) && ((t->a + s->b) == 1)))) {
						/*
						* while  
						*/
						
						DecompileIndent(*indent);
						fprintf(Decompileofile, "while ( %s ) {\n\n", arg1);
						(*indent)++;
						
					} else {
					/*
					* pure if  
						*/
						
						DecompileIndent(*indent);
						/*
						* fprintf(Decompileofile,"if ( %s ) { //3\n\n",arg1);
						*/
						fprintf(Decompileofile, "if ( %s ) {\n\n", arg1);
						(*indent)++;
					}
#endif
					
					if ((t->a + s->b) > 1) {
					/*
					* pure if  
						*/
						
						DecompileIndent(*indent);
						/*
						fprintf(Decompileofile, "if ( %s ) { //3\n\n", arg1);
						*/
						fprintf(Decompileofile, "if ( %s ) {\n\n", arg1);
						(*indent)++;
					} else {
						
						dum = 1;
						for (k = t + (t->a); k < s; k++) {
							tom = k->op % 100;
							if (tom == OP_GOTO || tom == OP_IF || tom == OP_IFNOT)
								dum = 0;
						}
						if (dum) {	/*
									* while 
							*/
							
							DecompileIndent(*indent);
							fprintf(Decompileofile, "while ( %s ) {\n\n", arg1);
							(*indent)++;
						} else {
						/*
						* pure if  
							*/
							
							DecompileIndent(*indent);
							/*
							fprintf(Decompileofile, "if ( %s ) { //3\n\n", arg1);
							*/
							fprintf(Decompileofile, "if ( %s ) {\n\n", arg1);
							(*indent)++;
						}
					}
				}
			}
			
	} else {
	/*
	* do ... while 
		*/
		
		(*indent)--;
		fprintf(Decompileofile, "\n");
		DecompileIndent(*indent);
		fprintf(Decompileofile, "} while ( %s );\n", arg1);
		
	}
	
    } else if (s->op == OP_GOTO) {
		
		if (s->a > 0) {
		/*
		* else 
			*/
			(*indent)--;
			fprintf(Decompileofile, "\n");
			DecompileIndent(*indent);
			fprintf(Decompileofile, "} else {\n\n");
			(*indent)++;
			
		} else {
		/*
		* while 
			*/
			(*indent)--;
			fprintf(Decompileofile, "\n");
			DecompileIndent(*indent);
			fprintf(Decompileofile, "}\n");
			
		}
		
    } else {
		fprintf(Decompileofile, "\n/* Warning: UNKNOWN COMMAND */\n");
    }
	
	/*
    printf("DecompileDecompileStatement - Current line is \"%s\"\n", line);
	*/
	
    if (arg1)
		free(arg1);
    if (arg2)
		free(arg2);
    if (arg3)
		free(arg3);
	
    return;
}

void 
DecompileDecompileFunction(dfunction_t * df)
{
    dstatement_t *ds;
    int indent;
	
    /*
	* Initialize 
	*/
    DecompileImmediate(df, 0, 0, NULL);
	
    indent = 1;
	
    ds = statements + df->first_statement;
    while (1) {
		DecompileDecompileStatement(df, ds, &indent);
		if (!ds->op)
			break;
		ds++;
    }
	
    if (indent != 1)
		fprintf(Decompileofile, "/* Warning : Indentiation structure corrupt */\n");
	
}

char *
DecompileString(char *string)
{
    static char buf[255];
    char *s;
    int c = 1;
	
    s = buf;
    *s++ = '"';
    while (string && *string) {
		if (c == sizeof(buf) - 2)
			break;
		if (*string == '\n') {
			*s++ = '\\';
			*s++ = 'n';
			c++;
		} else if (*string == '"') {
			*s++ = '\\';
			*s++ = '"';
			c++;
		} else {
			*s++ = *string;
			c++;
		}
		string++;
		if (c > (int)(sizeof(buf) - 10)) {
			*s++ = '.';
			*s++ = '.';
			*s++ = '.';
			c += 3;
			break;
		}
    }
    *s++ = '"';
    *s++ = 0;
    return buf;
}

char *
DecompileValueString(etype_t type, void *val)
{
    static char line[1024];
	
    line[0] = '\0';
	
    switch (type) {
    case ev_string:
		sprintf(line, "%s", DecompileString(strings + *(int *)val));
		break;
    case ev_void:
		sprintf(line, "void");
		break;
    case ev_float:
		sprintf(line, "%5.3f", *(float *)val);
		break;
    case ev_vector:
		sprintf(line, "'%.3f %5.3f %5.3f'", ((float *)val)[0], ((float *)val)[1], ((float *)val)[2]);
		break;
    default:
		sprintf(line, "bad type %i", type);
		break;
    }
	
    return line;
}

char *
DecompilePrintParameter(ddef_t * def)
{
    static char line[128];
	
    line[0] = '0';
	
    if (!strcmp(strings + def->s_name, "IMMEDIATE")) {
		sprintf(line, "%s", DecompileValueString(def->type, &pr_globals[def->ofs]));
    } else
		sprintf(line, "%s %s", type_names[def->type], strings + def->s_name);
	
    return line;
}

ddef_t *
GetField(char *name)
{
    int i;
    ddef_t *d;
	
    for (i = 1; i < numfielddefs; i++) {
		d = &fields[i];
		
		if (!strcmp(strings + d->s_name, name))
			return d;
    }
    return NULL;
}

ddef_t *
DecompileGetParameter(gofs_t ofs)
{
    int i;
    ddef_t *def;
	
    def = NULL;
	
    for (i = 0; i < numglobaldefs; i++) {
		def = &globals[i];
		
		if (def->ofs == ofs) {
			return def;
		}
    }
    return NULL;
}

void 
DecompileFunction(char *name)
{
    int i, findex, ps;
    dstatement_t *ds, *ts;
    dfunction_t *df;
    ddef_t *par;
    char *arg2;
    unsigned short dom, tom;
	
    int j, start, end;
    dfunction_t *dfpred;
    ddef_t *ef;
	
    static char line[256];
	
    dstatement_t *k;
    int dum;
	
    for (i = 1; i < numfunctions; i++)
		if (!strcmp(name, strings + functions[i].s_name))
			break;
		if (i == numfunctions)
			Error("No function named \"%s\"", name);
		df = functions + i;
		
		findex = i;
		
		/*
		* Check ''local globals'' 
		*/
		
		dfpred = df - 1;
		
		for (j = 0, ps = 0; j < dfpred->numparms; j++)
			ps += dfpred->parm_size[j];
		
		start = dfpred->parm_start + dfpred->locals + ps;
		
		if (dfpred->first_statement < 0 && df->first_statement > 0)
			start -= 1;
		
		if (start == 0)
			start = 1;
		
		end = df->parm_start;
		
		for (j = start; j < end; j++) {
			
			par = DecompileGetParameter(j);
			
			if (par) {
				
				if (par->type & (1 << 15))
					par->type -= (1 << 15);
				
				if (par->type == ev_function) {
					
					if (strcmp(strings + par->s_name, "IMMEDIATE"))
						if (strcmp(strings + par->s_name, name)) {
							fprintf(Decompileofile, "%s;\n", DecompileProfiles[DecompileGetFunctionIdxByName(strings + par->s_name)]);
						}
				} else if (par->type != ev_pointer)
					if (strcmp(strings + par->s_name, "IMMEDIATE")) {
						
						if (par->type == ev_field) {
							
							ef = GetField(strings + par->s_name);
							
							if (!ef)
								Error("Could not locate a field named \"%s\"", strings + par->s_name);
							
							if (ef->type == ev_vector)
								j += 3;
							
#ifndef DONT_USE_DIRTY_TRICKS
							if ((ef->type == ev_function) && !strcmp(strings + ef->s_name, "th_pain")) {
								fprintf(Decompileofile, ".void(entity attacker, float damage) th_pain;\n");
							} else
#endif
								fprintf(Decompileofile, ".%s %s;\n", type_names[ef->type], strings + ef->s_name);
							
						} else {
							
							if (par->type == ev_vector)
								j += 2;
							
							if (par->type == ev_entity || par->type == ev_void) {
								
								fprintf(Decompileofile, "%s %s;\n", type_names[par->type], strings + par->s_name);
								
							} else {
								
								line[0] = '\0';
								sprintf(line, "%s", DecompileValueString(par->type, &pr_globals[par->ofs]));
								
								if ((strlen(strings + par->s_name) > 1) &&
									isupper((strings + par->s_name)[0]) &&
									(isupper((strings + par->s_name)[1]) || (strings + par->s_name)[1] == '_')) {
									fprintf(Decompileofile, "%s %s    = %s;\n", type_names[par->type], strings + par->s_name, line);
								} else
									fprintf(Decompileofile, "%s %s /* = %s */;\n", type_names[par->type], strings + par->s_name, line);
							}
						}
					}
			}
		}
		/*
		* Check ''local globals'' 
		*/
		
		if (df->first_statement <= 0) {
			
			fprintf(Decompileofile, "%s", DecompileProfiles[findex]);
			fprintf(Decompileofile, " = #%i; \n", -df->first_statement);
			
			return;
		}
		ds = statements + df->first_statement;
		
		while (1) {
			
			dom = (ds->op) % 100;
			
			if (!dom)
				break;
			
			else if (dom == OP_GOTO) {
			/*
			* check for i-t-e 
				*/
				
				if (ds->a > 0) {
					ts = ds + ds->a;
					ts->op += 100;	/*
									* mark the end of a if/ite construct 
					*/
				}
			} else if (dom == OP_IFNOT) {
			/*
			* check for pure if 
				*/
				
				ts = ds + ds->b;
				tom = (ts - 1)->op % 100;
				
				if (tom != OP_GOTO)
				ts->op += 100;	/*
								* mark the end of a if/ite construct 
				*/
				else if ((ts - 1)->a < 0) {
					
				/*
				arg2 = DecompileGlobal(ds->a, NULL);
				
				  if (!((((ts - 1)->a + ds->b) == 0) ||
				  ((arg2 != NULL) && (((ts - 1)->a + ds->b) == 1))))
				  (ts - 1)->op += 100;
				  
					if (arg2)
					free(arg2);
					*/
					
					if (((ts - 1)->a + ds->b) > 1) {
					/*
					* pure if  
						*/
						ts->op += 100;	/*
										* mark the end of a if/ite construct 
						*/
					} else {
						
						dum = 1;
						for (k = (ts - 1) + ((ts - 1)->a); k < ds; k++) {
							tom = k->op % 100;
							if (tom == OP_GOTO || tom == OP_IF || tom == OP_IFNOT)
								dum = 0;
						}
						if (!dum) {
						/*
						* pure if  
							*/
							ts->op += 100;	/*
											* mark the end of a if/ite construct 
							*/
						}
					}
				}
			} else if (dom == OP_IF) {
				ts = ds + ds->b;
				ts->op += 10000;	/*
									* mark the start of a do construct 
				*/
			}
			ds++;
		}
		
		/*
		* print the prototype 
		*/
		fprintf(Decompileofile, "%s", DecompileProfiles[findex]);
		
		/*
		* handle state functions 
		*/
		
		ds = statements + df->first_statement;
		
		if (ds->op == OP_STATE) {
			
			par = DecompileGetParameter(ds->a);
			if (!par)
				Error("Error - Can't determine frame number.");
			
			arg2 = DecompileGet(df, ds->b, NULL);
			if (!arg2)
				Error("Error - No state parameter with offset %i.", ds->b);
			
			fprintf(Decompileofile, " = [ %s, %s ]", DecompileValueString(par->type, &pr_globals[par->ofs]), arg2);
			
			free(arg2);
			
		} else {
			fprintf(Decompileofile, " =");
		}
		fprintf(Decompileofile, " {\n\n");
		
		/*
		fprintf(Decompileprofile, "%s", DecompileProfiles[findex]);
		fprintf(Decompileprofile, ") %s;\n", name);
		*/
		
		/*
		* calculate the parameter size 
		*/
		
		for (j = 0, ps = 0; j < df->numparms; j++)
			ps += df->parm_size[j];
		
			/*
			* print the locals 
		*/
		
		if (df->locals > 0) {
			
			if ((df->parm_start) + df->locals - 1 >= (df->parm_start) + ps) {
				
				for (i = df->parm_start + ps; i < (df->parm_start) + df->locals; i++) {
					
					par = DecompileGetParameter(i);
					
					if (!par) {
						fprintf(Decompileofile, "   /* Warning: No local name with offset %i */\n", i);
					} else {
						
						if (par->type == ev_function)
							fprintf(Decompileofile, "   /* Warning: Fields and functions must be global */\n");
						else
							fprintf(Decompileofile, "   local %s;\n", DecompilePrintParameter(par));
						if (par->type == ev_vector)
							i += 2;
					}
				}
				
				fprintf(Decompileofile, "\n");
				
			}
		}
		/*
		* do the hard work 
		*/
		
		DecompileDecompileFunction(df);
		
		fprintf(Decompileofile, "\n};\n");
}

void 
DecompileDecompileFunctions(void)
{
    int i;
    dfunction_t *d;
    FILE *f;
    char fname[512];
	char shownwarning = 0;
	
    DecompileCalcProfiles();
	
    Decompileprogssrc = fopen("progs.src", "w");
    if (!Decompileprogssrc)
		Error("Fatal Error - Could not open \"progs.src\" for output.");
	
    fprintf(Decompileprogssrc, "./progs.dat\n\n");
	
    for (i = 1; i < numfunctions; i++) {
		d = &functions[i];
		
		fname[0] = '\0';
		sprintf(fname, "%s", strings + d->s_file);
		
		f = fopen(fname, "a+");
		
        if (!f) {
			sprintf(fname, "scram.qc");
			if (!shownwarning) {
				ShowWarningEntry("Warning - Scrambled file, generating \"%s\".", fname);
				shownwarning = 1;
			}
            f = fopen(fname, "a+");
            //Error("Fatal Error - Could not open \"%s\" for output.", fname);
        }

		if (!DecompileAlreadySeen(fname)) 
			fprintf(Decompileprogssrc, "%s\n", fname);
		
		Decompileofile = f;
		DecompileFunction(strings + d->s_name);
		
		if (fclose(f))
			Error("Fatal Error - Could not close \"%s\" properly.", fname);
    }
	
    if (fclose(Decompileprogssrc))
		Error("Fatal Error - Could not close \"progs.src\" properly.");
		/*
		if (fclose(Decompileprofile))
		Error("DecompileDecompileFunctions - Could not close \"!profile.qc\" properly.");
	*/
}

void 
DecompileProgsDat(char *name)
{
	
    DecompileReadData(name);
    DecompileDecompileFunctions();
	
}

char *
DecompileGlobalStringNoContents(gofs_t ofs)
{
    int i;
    ddef_t *def;
    static char line[128];
	
    line[0] = '0';
    sprintf(line, "%i(???)", ofs);
	
    for (i = 0; i < numglobaldefs; i++) {
		def = &globals[i];
		
		if (def->ofs == ofs) {
			line[0] = '0';
			sprintf(line, "%i(%s)", def->ofs, strings + def->s_name);
			break;
		}
    }
	
    i = strlen(line);
    for (; i < 16; i++)
		strcat(line, " ");
    strcat(line, " ");
	
    return line;
}

char *
DecompileGlobalString(gofs_t ofs)
{
    char *s;
    int i;
    ddef_t *def;
    static char line[128];
	
    line[0] = '0';
    sprintf(line, "%i(???)", ofs);
	
    for (i = 0; i < numglobaldefs; i++) {
		def = &globals[i];
		
		if (def->ofs == ofs) {
			
			line[0] = '0';
			if (!strcmp(strings + def->s_name, "IMMEDIATE")) {
				s = PR_ValueString(def->type, &pr_globals[ofs]);
				sprintf(line, "%i(%s)", def->ofs, s);
			} else
				sprintf(line, "%i(%s)", def->ofs, strings + def->s_name);
		}
    }
	
    i = strlen(line);
    for (; i < 16; i++)
		strcat(line, " ");
    strcat(line, " ");
	
    return line;
}

void 
DecompilePrintStatement(dstatement_t * s)
{
    int i;
	
    printf("%4i : %s ", (int)(s - statements), pr_opcodes[s->op].opname);
    i = strlen(pr_opcodes[s->op].opname);
    for (; i < 10; i++)
		printf(" ");
	
    if (s->op == OP_IF || s->op == OP_IFNOT)
		printf("%sbranch %i", DecompileGlobalString(s->a), s->b);
    else if (s->op == OP_GOTO) {
		printf("branch %i", s->a);
    } else if ((unsigned)(s->op - OP_STORE_F) < 6) {
		printf("%s", DecompileGlobalString(s->a));
		printf("%s", DecompileGlobalStringNoContents(s->b));
    } else {
		if (s->a)
			printf("%s", DecompileGlobalString(s->a));
		if (s->b)
			printf("%s", DecompileGlobalString(s->b));
		if (s->c)
			printf("%s", DecompileGlobalStringNoContents(s->c));
    }
    printf("\n");
}

void 
DecompilePrintFunction(char *name)
{
    int i;
    dstatement_t *ds;
    dfunction_t *df;
	
    for (i = 0; i < numfunctions; i++)
		if (!strcmp(name, strings + functions[i].s_name))
			break;
		if (i == numfunctions)
			Error("No function names \"%s\"", name);
		df = functions + i;
		
		printf("Statements for %s:\n", name);
		ds = statements + df->first_statement;
		while (1) {
			DecompilePrintStatement(ds);
			
			if (!ds->op)
				break;
			ds++;
		}
}
