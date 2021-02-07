#ifndef __ES_BRIDGE_H__
#define __ES_BRIDGE_H__

#include "common.h"
#include "duktape.h"

typedef duk_context* LPDUKCONTEXT;

WORD PAL_ExecuteECMAScript(LPDUKCONTEXT ctx, WORD wScriptID);

VOID PAL_ExecuteECMAScriptBeforeStart(LPDUKCONTEXT ctx);

VOID PAL_InitESHandlers(LPDUKCONTEXT ctx);


#endif