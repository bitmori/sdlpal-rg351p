#include "main.h"

static duk_ret_t native_print(duk_context *ctx) {
	// UTIL_LogOutput(LOGLEVEL_ERROR, "%s\n", duk_to_string(ctx, 0));
	printf("%s\n", duk_to_string(ctx, 0));
  	return 0;  /* no return value (= undefined) */
}

static duk_ret_t set_exp_multiplier(LPDUKCONTEXT ctx) {
   int em = duk_to_int(ctx, 0);
   gpGlobals->nExpMultiplier = abs(em);
   return 0;
}

static duk_ret_t give_cash(LPDUKCONTEXT ctx) {
   int money = 1000;
   int argc = duk_get_top(ctx);  /* #args */
   if (argc != 0) {
       money = duk_to_int(ctx, 0);
   }
   gpGlobals->dwCash += money;
   return 0;
}

static duk_ret_t lock_team(LPDUKCONTEXT ctx) {
   gpGlobals->fLockTeamMember = TRUE;
   return 0;
}

static duk_ret_t unlock_team(LPDUKCONTEXT ctx) {
   gpGlobals->fLockTeamMember = FALSE;
   return 0;
}

static duk_ret_t add_inventory(LPDUKCONTEXT ctx) {
   int argc = duk_get_top(ctx);
   if (argc < 2) {
       return -1;
   }
   int id = duk_to_int(ctx, 0);
   int cn = duk_to_int(ctx, 1);
   PAL_AddItemToInventory(id, cn);
   return 0;
}

static duk_ret_t make_fantastic4(LPDUKCONTEXT ctx)
{
   //
   // Set the player party
   //
   gpGlobals->wMaxPartyMemberIndex = 3;
   gpGlobals->rgParty[0].wPlayerRole = 0; // => XY
   gpGlobals->rgParty[1].wPlayerRole = 1; // => 02
   gpGlobals->rgParty[2].wPlayerRole = 4; // => ANU
   gpGlobals->rgParty[3].wPlayerRole = 2; // => Miss
   g_Battle.rgPlayer[0].action.ActionType = kBattleActionAttack;
   g_Battle.rgPlayer[1].action.ActionType = kBattleActionAttack;
   g_Battle.rgPlayer[2].action.ActionType = kBattleActionAttack;
   g_Battle.rgPlayer[3].action.ActionType = kBattleActionAttack;

   if (gpGlobals->wMaxPartyMemberIndex == 0)
   {
      // HACK for Dream 2.11
      gpGlobals->rgParty[0].wPlayerRole = 0;
      gpGlobals->wMaxPartyMemberIndex = 1;
   }

   //
   // Reload the player sprites
   //
   PAL_SetLoadFlags(kLoadPlayerSprite);
   PAL_LoadResources();

   memset(gpGlobals->rgPoisonStatus, 0, sizeof(gpGlobals->rgPoisonStatus));
   PAL_UpdateEquipments();
   return 0;
}

static duk_ret_t rename_game_object(LPDUKCONTEXT ctx) {
    // rename_game_object(object_id, [words]);
    int argc = duk_get_top(ctx);
    if (argc != 2) {
        return -1;
    }
    if (!duk_is_array(ctx, 1)) {
        return -1;
    }
    int w_id = duk_to_int(ctx, 0);
    if (w_id >= g_TextLib.nWords) {
        return -1;
    }
    size_t n = min(5, duk_get_length(ctx, 1));
    if (n == 0) {
        return -1;
    }
    WCHAR new_name[5] = {0x2020};
    for (int i = 0; i < n; i++) {
        duk_get_prop_index(ctx, 1, i);
        new_name[i] = duk_to_int(ctx, -1);
        duk_pop(ctx);
    }
    memcpy(g_TextLib.lpWordBuf[w_id], new_name, sizeof(new_name));
    return 0;
}

static duk_ret_t set_game_script(LPDUKCONTEXT ctx) {
    return 0;
}

static duk_ret_t get_game_script(LPDUKCONTEXT ctx) {
    return 0;
}

static duk_ret_t set_game_event_obj(LPDUKCONTEXT ctx) {
    return 0;
}
static duk_ret_t get_game_event_obj(LPDUKCONTEXT ctx) {
    return 0;
}

static duk_ret_t set_game_obj_item(LPDUKCONTEXT ctx) {
    return 0;
}
static duk_ret_t get_game_obj_item(LPDUKCONTEXT ctx) {
    return 0;
}

static duk_ret_t set_game_obj_enemy(LPDUKCONTEXT ctx) {
    return 0;
}
static duk_ret_t get_game_obj_enemy(LPDUKCONTEXT ctx) {
    return 0;
}

static duk_ret_t set_game_obj_poison(LPDUKCONTEXT ctx) {
    return 0;
}
static duk_ret_t get_game_obj_poison(LPDUKCONTEXT ctx) {
    return 0;
}


static duk_ret_t set_game_store(LPDUKCONTEXT ctx) {
    return 0;
}
static duk_ret_t get_game_store(LPDUKCONTEXT ctx) {
    return 0;
}

static duk_ret_t set_game_enemy(LPDUKCONTEXT ctx) {
    return 0;
}
static duk_ret_t get_game_enemy(LPDUKCONTEXT ctx) {
    return 0;
}

static duk_ret_t set_game_enemy_team(LPDUKCONTEXT ctx) {
    return 0;
}
static duk_ret_t get_game_enemy_team(LPDUKCONTEXT ctx) {
    return 0;
}

static duk_ret_t set_game_obj_magic(LPDUKCONTEXT ctx) {
    return 0;
}

static duk_ret_t get_game_lvlup_magic(LPDUKCONTEXT ctx) {
    int w = duk_to_int(ctx, 0);
    if (w >= MAX_PLAYABLE_PLAYER_ROLES) {
        return 0;
    }
    int j = 0;
    while (j < gpGlobals->g.nLevelUpMagic)
    {
         if (gpGlobals->g.lprgLevelUpMagic[j].m[w].wMagic == 0)
         {
            j++;
            continue;
         }
         int lv = gpGlobals->g.lprgLevelUpMagic[j].m[w].wLevel;
         int id = gpGlobals->g.lprgLevelUpMagic[j].m[w].wMagic;
         printf("%d@Lv%d #%d = %ls\n", w, lv, id, PAL_GetWord(id));
         j++;
    }
}

static duk_ret_t get_game_obj_magic(LPDUKCONTEXT ctx) {
    int obj_m_id = duk_to_int(ctx, 0);
    int magic_id = gpGlobals->g.rgObject[obj_m_id].magic.wMagicNumber;
    int script_on_use = gpGlobals->g.rgObject[obj_m_id].magic.wScriptOnUse;
    int script_on_ok = gpGlobals->g.rgObject[obj_m_id].magic.wScriptOnSuccess;
    int flags = gpGlobals->g.rgObject[obj_m_id].magic.wFlags;
    printf("#%d = %ls [MagicID: %d, OnUse: %d, OnOK: %d, Flags: %x]\n", obj_m_id, PAL_GetWord(obj_m_id), 
            magic_id, script_on_use, script_on_ok, flags);
    LPMAGIC magic = &(gpGlobals->g.lprgMagic[magic_id]);
    printf("Efx = %d, Type = %d, (x, y) = (%d, %d), SummonEfx = %d, Speed = %d, EfxTimes = %d, (S,W) = (%d, %d), Cost = %d, BaseDmg = %d, Elem = %d, Sound = %d\n", 
    magic->wEffect, magic->wType, magic->wXOffset, magic->wYOffset, magic->wSummonEffect, magic->wSpeed,
    magic->wEffectTimes, magic->wShake, magic->wWave, magic->wCostMP, magic->wBaseDamage, magic->wElemental, magic->wSound);
    return 0;
}

static duk_ret_t add_game_magic(LPDUKCONTEXT ctx) {
    return 0;
}

static duk_ret_t set_game_magic(LPDUKCONTEXT ctx) {
    return 0;
}
static duk_ret_t get_game_magic(LPDUKCONTEXT ctx) {
    int m_id = duk_to_int(ctx, 0);
    if (m_id >= gpGlobals->g.nMagic) {
        return 0;
    }
    LPMAGIC magic = &(gpGlobals->g.lprgMagic[m_id]);
    // printf("Efx = %d, Type = %d, (x, y) = (%d, %d), SummonEfx = %d, Speed = %d, ", magic->wBaseDamage, magic->wCostMP, magic->);
    return 0;
}

static duk_ret_t set_game_player_data(LPDUKCONTEXT ctx) {
    return 0;
}
static duk_ret_t get_game_player_data(LPDUKCONTEXT ctx) {
    return 0;
}

static char * va(const char *format, ...) {
   static char string[256];
   va_list     argptr;

   va_start(argptr, format);
   vsnprintf(string, 256, format, argptr);
   va_end(argptr);

   return string;
}

WORD PAL_ExecuteECMAScript(LPDUKCONTEXT ctx, WORD wScriptID) {
   if (gpGlobals->duk == NULL)
   {
      return -1;
   }
   FILE *file = fopen(va("%s%d%s", PAL_SAVE_PREFIX, wScriptID, ".js"), "r");
   if (file == NULL)
   {
      return -1;
   }
   char buf[0xFFFF];
   size_t len = fread((void *) buf, 1, sizeof(buf), file);
   fclose(file);
   duk_push_lstring(ctx, (const char *) buf, (duk_size_t) len);
   if (duk_peval(ctx) != 0) {
    printf("Script error: %s\n", duk_safe_to_string(ctx, -1));
   }
   duk_pop(ctx);
   return 0;
}

VOID PAL_ExecuteECMAScriptBeforeStart(LPDUKCONTEXT ctx) {
    // Inject/modify item here
    int wMagicId = 0;
    // gpGlobals->g.lprgMagic[wMagicId].
}

static void sdlpal_add_func(duk_context *ctx, duk_c_function func, const char *name, int argc) {
	duk_push_c_function(ctx, func, argc);
	duk_push_string(ctx, "name");
	duk_push_string(ctx, name);
	duk_def_prop(ctx, -3, DUK_DEFPROP_HAVE_VALUE | DUK_DEFPROP_FORCE);
	duk_put_prop_string(ctx, -2, name);
}


VOID PAL_InitESHandlers(LPDUKCONTEXT ctx) {
   duk_push_c_function(ctx, native_print, 1);
   duk_put_global_string(ctx, "print");
   
   duk_push_object(ctx); // add an empty object as Sdlpal
   sdlpal_add_func(ctx, give_cash, "give_cash", 1);
   sdlpal_add_func(ctx, make_fantastic4, "make_team", 0);
   sdlpal_add_func(ctx, lock_team, "lock_team", 0);
   sdlpal_add_func(ctx, unlock_team, "unlock_team", 0);
   sdlpal_add_func(ctx, add_inventory, "add_item", 2);
   sdlpal_add_func(ctx, set_exp_multiplier, "set_exp_multiplier", 1);
   sdlpal_add_func(ctx, rename_game_object, "rename_game_obj", 2);
   sdlpal_add_func(ctx, get_game_obj_magic, "get_obj", 1);
   sdlpal_add_func(ctx, get_game_lvlup_magic, "get_lvlup_magic", 1);
   duk_put_global_string(ctx, "Sdlpal");  // register 'Sdlpal' as a global object
}
