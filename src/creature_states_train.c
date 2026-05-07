/******************************************************************************/
// Free implementation of Bullfrog's Dungeon Keeper strategy game.
/******************************************************************************/
/** @file creature_states_train.c
 *     Creature state machine functions for their job in various rooms.
 * @par Purpose:
 *     Defines elements of states[] array, containing valid creature states.
 * @par Comment:
 *     None.
 * @author   KeeperFX Team
 * @date     23 Sep 2009 - 05 Jan 2011
 * @par  Copying and copyrights:
 *     This program is free software; you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation; either version 2 of the License, or
 *     (at your option) any later version.
 */
/******************************************************************************/
#include "pre_inc.h"
#include "creature_states_train.h"
#include "globals.h"

#include "bflib_math.h"

#include "creature_states.h"
#include "creature_states_combt.h"
#include "creature_states_mood.h"
#include "creature_instances.h"
#include "thing_list.h"
#include "creature_control.h"
#include "config_creature.h"
#include "config_rules.h"
#include "config_terrain.h"
#include "player_utils.h"
#include "thing_stats.h"
#include "thing_physics.h"
#include "thing_objects.h"
#include "thing_effects.h"
#include "thing_creature.h"
#include "thing_navigate.h"
#include "room_data.h"
#include "room_jobs.h"
#include "map_utils.h"
#include "ariadne_wallhug.h"
#include "gui_soundmsgs.h"
#include "game_legacy.h"
#include "post_inc.h"

/******************************************************************************/
/** Returns if the creature meets conditions to be trained.
 *
 * @param thing The creature thing to be tested.
 * @return
 */
TbBool creature_can_be_trained(const struct Thing *thing)
{
    struct CreatureModelConfig* crconf = creature_stats_get_from_thing(thing);
    struct CreatureControl* cctrl = creature_control_get_from_thing(thing);
    // Creatures without training value can't be trained
    if (crconf->training_value <= 0)
        return false;
    if ((cctrl->exp_level >= game.conf.rules[thing->owner].rooms.training_room_max_level-1) &! (game.conf.rules[thing->owner].rooms.training_room_max_level == 0))
        return false;
    // If its model can train, check if this one can gain more experience
    return creature_can_gain_experience(thing);
}

TbBool player_can_afford_to_train_creature(const struct Thing *thing)
{
    struct Dungeon* dungeon = get_dungeon(thing->owner);
    GoldAmount training_cost = calculate_correct_creature_training_cost(thing);
    return (dungeon->total_money_owned >= training_cost);
}

#define ARENA_MAX_LEVEL 12
#define ARENA_EXP_GAIN_SCALE 48
#define ARENA_DUEL_HIT_INTERVAL 480

TbBool creature_can_do_arena(const struct Thing *thing)
{
    if (!thing_is_creature(thing))
        return false;
    if (thing_is_creature_digger(thing))
        return false;
    long imp_model = get_id(creature_desc, "IMP");
    if ((imp_model > 0) && (thing->model == imp_model))
        return false;
    long zombie_model = get_id(creature_desc, "ZOMBIE");
    if ((zombie_model > 0) && (thing->model == zombie_model))
        return false;
    return true;
}

long count_creatures_working_in_arena_room(const struct Room *room)
{
    if (room_is_invalid(room))
        return 0;
    long count = 0;
    long i = room->creatures_list;
    unsigned long k = 0;
    while (i != 0)
    {
        struct Thing* thing = thing_get(i);
        if (!thing_exists(thing))
            break;
        struct CreatureControl* cctrl = creature_control_get_from_thing(thing);
        if (!creature_control_exists(cctrl))
            break;
        i = cctrl->next_in_room;
        CrtrStateId state = get_creature_state_besides_move(thing);
        if (((state == CrSt_AtArenaRoom) || (state == CrSt_ArenaDuel)) && creature_can_do_arena(thing))
            count++;
        k++;
        if (k > THINGS_COUNT)
        {
            ERRORLOG("Infinite loop detected when counting arena creatures");
            break;
        }
    }
    return count;
}

TbBool creature_can_gain_arena_experience(const struct Thing *thing)
{
    if (!creature_can_do_arena(thing))
        return false;
    struct CreatureControl* cctrl = creature_control_get_from_thing(thing);
    if (creature_control_invalid(cctrl))
        return false;
    return cctrl->exp_level < ARENA_MAX_LEVEL - 1;
}

long arena_experience_required_for_next_level(const struct Thing *thing)
{
    struct CreatureControl* cctrl = creature_control_get_from_thing(thing);
    struct CreatureModelConfig* crconf = creature_stats_get_from_thing(thing);
    long exp_level = cctrl->exp_level;
    long base_exp;
    if (exp_level < CREATURE_NORMAL_MAX_LEVEL - 1)
    {
        base_exp = crconf->to_level[exp_level];
    } else
    {
        base_exp = crconf->to_level[CREATURE_NORMAL_MAX_LEVEL - 2];
        long extra_level = exp_level - (CREATURE_NORMAL_MAX_LEVEL - 2);
        if (extra_level < 1)
            extra_level = 1;
        base_exp *= extra_level + 1;
    }
    if (base_exp <= 0)
        base_exp = 1000;
    return base_exp << 8;
}

TbBool arena_update_creature_level(struct Thing *thing)
{
    TbBool leveled = false;
    struct CreatureControl* cctrl = creature_control_get_from_thing(thing);
    while (creature_can_gain_arena_experience(thing))
    {
        long required = arena_experience_required_for_next_level(thing);
        if (cctrl->exp_points < required)
            break;
        cctrl->exp_points -= required;
        remove_creature_score_from_owner(thing);
        set_creature_level(thing, cctrl->exp_level + 1);
        leveled = true;
    }
    if (!creature_can_gain_arena_experience(thing))
        cctrl->exp_points = 0;
    return leveled;
}

TbBool arena_raise_creature_one_level(struct Thing *thing)
{
    if (!creature_can_gain_arena_experience(thing))
        return false;
    struct CreatureControl* cctrl = creature_control_get_from_thing(thing);
    remove_creature_score_from_owner(thing);
    set_creature_level(thing, cctrl->exp_level + 1);
    cctrl->exp_points = 0;
    return true;
}

void clear_arena_partner(struct Thing *thing)
{
    struct CreatureControl* cctrl = creature_control_get_from_thing(thing);
    if (creature_control_invalid(cctrl))
        return;
    cctrl->training.mode = CrTrMd_SearchForTrainPost;
    cctrl->training.partner_idx = 0;
    cctrl->training.partner_creation = 0;
    cctrl->training.train_timeout = 0;
    cctrl->wait_to_turn = 0;
}

void finish_creature_arena_session(struct Thing *thing)
{
    struct CreatureControl* cctrl = creature_control_get_from_thing(thing);
    clear_arena_partner(thing);
    remove_creature_from_work_room(thing);
    if (external_set_thing_state(thing, CrSt_CreatureBeHappy)) {
        cctrl->countdown = 50;
    } else {
        set_start_state(thing);
    }
}

void setup_training_move(struct Thing *creatng, SubtlCodedCoords stl_num)
{
    struct CreatureControl* cctrl = creature_control_get_from_thing(creatng);
    cctrl->moveto_pos.x.val = subtile_coord_center(stl_num_decode_x(stl_num));
    cctrl->moveto_pos.y.val = subtile_coord_center(stl_num_decode_y(stl_num));
    cctrl->moveto_pos.z.val = get_thing_height_at(creatng, &cctrl->moveto_pos);
    if (thing_in_wall_at(creatng, &cctrl->moveto_pos))
    {
        ERRORLOG("Illegal setup to wall at (%d,%d)",
            (int)cctrl->moveto_pos.x.stl.num, (int)cctrl->moveto_pos.y.stl.num);
        set_start_state(creatng);
    }
    SYNCDBG(18,"The %s is moving to (%d,%d)", thing_model_name(creatng),
        (int)cctrl->moveto_pos.x.stl.num, (int)cctrl->moveto_pos.y.stl.num);
}

void setup_training_move_near(struct Thing *creatng, SubtlCodedCoords stl_num)
{
    MapSubtlCoord stl_x = stl_num_decode_x(stl_num);
    MapSubtlCoord stl_y = stl_num_decode_y(stl_num);
    // Select a subtile closer to current position
    MapSubtlDelta dist_x = stl_x - (MapSubtlDelta)creatng->mappos.x.stl.num;
    MapSubtlDelta dist_y = stl_y - (MapSubtlDelta)creatng->mappos.y.stl.num;
    if (abs(dist_x) > abs(dist_y))
    {
        if (dist_x > 0) {
            stl_x -= 1;
        } else {
            stl_x += 1;
        }
    } else
    {
        if (dist_y > 0) {
            stl_y -= 1;
        } else {
            stl_y += 1;
        }
    }
    SubtlCodedCoords near_stl_num = get_subtile_number(stl_x, stl_y);
    setup_training_move(creatng, near_stl_num);
}

struct Thing *get_creature_in_training_room_which_could_accept_partner(struct Room *room, struct Thing *partnertng)
{
    TRACE_THING(partnertng);
    long i = room->creatures_list;
    unsigned long k = 0;
    while (i != 0)
    {
        struct Thing* thing = thing_get(i);
        TRACE_THING(thing);
        struct CreatureControl* cctrl = creature_control_get_from_thing(thing);
        if (!creature_control_exists(cctrl))
        {
            ERRORLOG("Jump to invalid creature %d detected",(int)i);
            break;
        }
        i = cctrl->next_in_room;
        // Per creature code
        if (thing != partnertng)
        {
            if ( (get_creature_state_besides_move(thing) == CrSt_Training) && (cctrl->training.partner_idx == 0) )
            {
                if (get_room_thing_is_on(thing) == room) {
                    return thing;
                } else {
                    WARNLOG("The %s pretends to be in room but it's not.",thing_model_name(thing));
                }
            }
        }
        // Per creature code ends
        k++;
        if (k > THINGS_COUNT)
        {
          ERRORLOG("Infinite loop detected when sweeping creatures list");
          break;
        }
    }
    return INVALID_THING;
}

void setup_move_to_new_training_position(struct Thing *thing, struct Room *room, unsigned long restart)
{
    struct Coord3d pos;
    SYNCDBG(8,"Starting for %s",thing_model_name(thing));
    struct CreatureControl* cctrl = creature_control_get_from_thing(thing);
    struct CreatureModelConfig* crconf = creature_stats_get_from_thing(thing);
    if ( restart )
      cctrl->training.search_timeout = 50;
    // Try partner training
    if ((crconf->partner_training > 0) && (THING_RANDOM(thing, 100) < crconf->partner_training))
    {
        struct Thing* prtng = get_creature_in_training_room_which_could_accept_partner(room, thing);
        if (!thing_is_invalid(prtng))
        {
            SYNCDBG(7,"The %s found %s as training partner.",thing_model_name(thing),thing_model_name(prtng));
            struct CreatureControl* prctrl = creature_control_get_from_thing(prtng);
            prctrl->training.mode = CrTrMd_PartnerTraining;
            prctrl->training.train_timeout = 75;
            prctrl->training.partner_idx = thing->index;
            prctrl->training.partner_creation = thing->creation_turn;
            cctrl->training.mode = CrTrMd_PartnerTraining;
            cctrl->training.train_timeout = 75;
            cctrl->training.partner_idx = prtng->index;
            cctrl->training.partner_creation = prtng->creation_turn;
            return;
      }
    }
    // No partner - train at some random position
    cctrl->training.mode = CrTrMd_SearchForTrainPost;
    if (find_random_valid_position_for_thing_in_room(thing, room, &pos))
    {
        SYNCDBG(8,"Going to train at (%d,%d)",(int)pos.x.stl.num,(int)pos.y.stl.num);
        long i = get_subtile_number(pos.x.stl.num, pos.y.stl.num);
        setup_training_move(thing, i);
    } else {
        SYNCDBG(8,"No new position found, staying at (%d,%d)",(int)cctrl->moveto_pos.x.stl.num,(int)cctrl->moveto_pos.x.stl.num);
    }
    if (cctrl->instance_id == CrInst_NULL)
    {
        set_creature_instance(thing, CrInst_SWING_WEAPON_SWORD, 0, 0);
    }
}

struct Thing *get_creature_in_arena_room_which_could_accept_partner(struct Room *room, struct Thing *partnertng)
{
    TRACE_THING(partnertng);
    long i = room->creatures_list;
    unsigned long k = 0;
    while (i != 0)
    {
        struct Thing* thing = thing_get(i);
        TRACE_THING(thing);
        struct CreatureControl* cctrl = creature_control_get_from_thing(thing);
        if (!creature_control_exists(cctrl))
        {
            ERRORLOG("Jump to invalid creature %d detected",(int)i);
            break;
        }
        i = cctrl->next_in_room;
        if (thing != partnertng)
        {
            if ((get_creature_state_besides_move(thing) == CrSt_ArenaDuel) && (cctrl->training.partner_idx == 0) && creature_can_gain_arena_experience(thing))
            {
                if ((get_room_thing_is_on(thing) == room) && creature_can_do_arena(thing)) {
                    return thing;
                } else {
                    WARNLOG("The %s pretends to be in arena but it's not.",thing_model_name(thing));
                }
            }
        }
        k++;
        if (k > THINGS_COUNT)
        {
            ERRORLOG("Infinite loop detected when sweeping creatures list");
            break;
        }
    }
    return INVALID_THING;
}

void setup_move_to_new_arena_position(struct Thing *thing, struct Room *room, unsigned long restart)
{
    struct Coord3d pos;
    SYNCDBG(8,"Starting for %s",thing_model_name(thing));
    struct CreatureControl* cctrl = creature_control_get_from_thing(thing);
    if (restart)
        cctrl->training.search_timeout = 50;
    cctrl->training.mode = CrTrMd_SearchForTrainPost;
    cctrl->training.partner_idx = 0;
    cctrl->training.partner_creation = 0;
    cctrl->wait_to_turn = 0;
    if (get_room_thing_is_on(thing) == room)
    {
        cctrl->moveto_pos.x.val = subtile_coord_center(thing->mappos.x.stl.num);
        cctrl->moveto_pos.y.val = subtile_coord_center(thing->mappos.y.stl.num);
        cctrl->moveto_pos.z.val = get_thing_height_at(thing, &cctrl->moveto_pos);
        return;
    }
    if (find_random_valid_position_for_thing_in_room(thing, room, &pos))
    {
        SYNCDBG(8,"Going to arena position (%d,%d)",(int)pos.x.stl.num,(int)pos.y.stl.num);
        long i = get_subtile_number(pos.x.stl.num, pos.y.stl.num);
        setup_training_move(thing, i);
    } else {
        SYNCDBG(8,"No new arena position found, staying at (%d,%d)",(int)cctrl->moveto_pos.x.stl.num,(int)cctrl->moveto_pos.y.stl.num);
    }
}

void setup_arena_duel_partner(struct Thing *thing, struct Thing *prtng)
{
    struct CreatureControl* cctrl = creature_control_get_from_thing(thing);
    struct CreatureControl* prctrl = creature_control_get_from_thing(prtng);
    cctrl->training.mode = CrTrMd_PartnerTraining;
    cctrl->training.partner_idx = prtng->index;
    cctrl->training.partner_creation = prtng->creation_turn;
    cctrl->training.train_timeout = 8;
    cctrl->turns_at_job = 0;
    cctrl->wait_to_turn = 0;
    prctrl->training.mode = CrTrMd_PartnerTraining;
    prctrl->training.partner_idx = thing->index;
    prctrl->training.partner_creation = thing->creation_turn;
    prctrl->training.train_timeout = 8;
    prctrl->turns_at_job = 0;
    prctrl->wait_to_turn = 0;
}

CrStateRet knock_out_arena_loser(struct Thing *thing, struct Thing *winner, struct Thing *loser, struct Room *room)
{
    struct CreatureControl* winctrl = creature_control_get_from_thing(winner);
    struct CreatureControl* losctrl = creature_control_get_from_thing(loser);
    TbBool thing_lost = (loser == thing);
    SYNCDBG(6,"Arena fight finished: %s knocked out %s",thing_model_name(winner),thing_model_name(loser));
    clear_arena_partner(winner);
    clear_arena_partner(loser);
    winctrl->turns_at_job = -1;
    losctrl->turns_at_job = 0;
    remove_creature_from_work_room(loser);
    loser->health = 1;
    make_creature_unconscious(loser);
    arena_raise_creature_one_level(winner);
    if (!creature_can_gain_arena_experience(winner) || (count_creatures_working_in_arena_room(room) <= 1))
    {
        finish_creature_arena_session(winner);
    } else
    {
        setup_move_to_new_arena_position(winner, room, false);
    }
    if (thing_lost)
        return CrStRet_Modified;
    return CrStRet_Modified;
}

void arena_duelist_damage_partner(struct Thing *thing, struct Thing *prtng)
{
    struct CreatureControl* cctrl = creature_control_get_from_thing(thing);
    if ((cctrl->turns_at_job % ARENA_DUEL_HIT_INTERVAL) != 0)
        return;
    long damage = calculate_melee_damage(thing, 0);
    if (damage < 1)
        damage = 1;
    apply_damage_to_thing_and_display_health(prtng, damage, thing->owner);
    if (creature_can_gain_arena_experience(thing))
    {
        struct Dungeon* dungeon = get_dungeon(thing->owner);
        struct CreatureControl* prctrl = creature_control_get_from_thing(prtng);
        struct CreatureModelConfig* prconf = creature_stats_get_from_thing(prtng);
        long exp_factor = prconf->exp_for_hitting;
        if (exp_factor < 1)
            exp_factor = 1;
        long exp_gained = (exp_factor + game.conf.crtr_conf.exp.exp_on_hitting_increase_on_exp * exp_factor * (long)prctrl->exp_level / 100) * ARENA_EXP_GAIN_SCALE;
        exp_gained <<= 8;
        cctrl->prev_exp_points = cctrl->exp_points;
        cctrl->exp_points += exp_gained;
        dungeon->total_experience_creatures_gained += exp_gained;
    }
}

CrStateRet process_creature_in_arena_room(struct Thing *thing, struct Room *room)
{
    struct CreatureControl* cctrl = creature_control_get_from_thing(thing);
    struct CreatureControl* prctrl;
    struct Thing* prtng;
    long dist;
    long i;
    SYNCDBG(8,"Starting %s arena mode %d",thing_model_name(thing),(int)cctrl->training.mode);
    cctrl->annoy_untrained_turn = 0;
    switch (cctrl->training.mode)
    {
    case CrTrMd_SearchForTrainPost:
        if (!creature_can_gain_arena_experience(thing))
        {
            finish_creature_arena_session(thing);
            return CrStRet_ResetOk;
        }
        if (cctrl->instance_id != CrInst_NULL)
            break;
        prtng = get_creature_in_arena_room_which_could_accept_partner(room, thing);
        if (!thing_is_invalid(prtng))
        {
            SYNCDBG(7,"The %s found %s as arena opponent.",thing_model_name(thing),thing_model_name(prtng));
            setup_arena_duel_partner(thing, prtng);
            break;
        }
        if (cctrl->training.search_timeout < 1)
        {
            setup_move_to_new_arena_position(thing, room, true);
            cctrl->training.search_timeout = 100;
            break;
        }
        cctrl->training.search_timeout--;
        i = creature_move_to(thing, &cctrl->moveto_pos, get_creature_speed(thing), 0, 0);
        if (i == 1)
        {
            setup_move_to_new_arena_position(thing, room, false);
        } else
        if (i == -1)
        {
            ERRORLOG("Cannot get to (%d,%d) in the arena room",(int)cctrl->moveto_pos.x.stl.num,(int)cctrl->moveto_pos.y.stl.num);
            remove_creature_from_work_room(thing);
            set_start_state(thing);
            return CrStRet_ResetFail;
        }
        break;
    case CrTrMd_PartnerTraining:
        if (cctrl->training.partner_idx == 0)
        {
            setup_move_to_new_arena_position(thing, room, false);
            return CrStRet_Modified;
        }
        if (!creature_can_gain_arena_experience(thing))
        {
            prtng = thing_get(cctrl->training.partner_idx);
            if (thing_exists(prtng))
                clear_arena_partner(prtng);
            finish_creature_arena_session(thing);
            return CrStRet_Modified;
        }
        prtng = thing_get(cctrl->training.partner_idx);
        TRACE_THING(prtng);
        if (!thing_exists(prtng) || (get_creature_state_besides_move(prtng) != CrSt_ArenaDuel) || (prtng->creation_turn != cctrl->training.partner_creation))
        {
            SYNCDBG(8,"The %s cannot continue arena duel - opponent is gone.",thing_model_name(thing));
            setup_move_to_new_arena_position(thing, room, false);
            return CrStRet_Modified;
        }
        prctrl = creature_control_get_from_thing(prtng);
        if (prctrl->training.partner_idx != thing->index)
        {
            SYNCDBG(6,"The %s cannot continue arena duel - %s changed opponent.",thing_model_name(thing),thing_model_name(prtng));
            cctrl->training.partner_idx = 0;
            setup_move_to_new_arena_position(thing, room, false);
            break;
        }
        if (get_room_thing_is_on(prtng) != room)
        {
            SYNCDBG(8,"The %s cannot continue arena duel - opponent left the room.",thing_model_name(thing));
            cctrl->training.partner_idx = 0;
            prctrl->training.partner_idx = 0;
            setup_move_to_new_arena_position(thing, room, false);
            break;
        }
        cctrl->turns_at_job++;
        dist = get_combat_distance(thing, prtng);
        if (dist > 284)
        {
            if (creature_move_to(thing, &prtng->mappos, get_creature_speed(thing), 0, 0) == -1)
            {
                WARNLOG("The %s cannot navigate to arena opponent",thing_model_name(thing));
                if (cctrl->instance_id == CrInst_NULL)
                    set_creature_instance(thing, CrInst_SWING_WEAPON_SWORD, 0, 0);
            }
        } else
        if (dist >= 156)
        {
            if (creature_turn_to_face(thing, &prtng->mappos) < DEGREES_10)
            {
                if ((cctrl->instance_id == CrInst_NULL) && ((get_gameturn() % 8) == 0))
                    set_creature_instance(thing, CrInst_SWING_WEAPON_SWORD, 0, 0);
                arena_duelist_damage_partner(thing, prtng);
                if (prtng->health < 0)
                    return knock_out_arena_loser(thing, thing, prtng, room);
                if (!creature_can_gain_arena_experience(thing))
                {
                    clear_arena_partner(prtng);
                    finish_creature_arena_session(thing);
                    return CrStRet_Modified;
                }
            }
        } else
        {
            if (creature_turn_to_face(thing, &prtng->mappos) < DEGREES_10)
            {
                if ((cctrl->instance_id == CrInst_NULL) && ((get_gameturn() % 8) == 0))
                    set_creature_instance(thing, CrInst_SWING_WEAPON_SWORD, 0, 0);
                arena_duelist_damage_partner(thing, prtng);
                if (prtng->health < 0)
                    return knock_out_arena_loser(thing, thing, prtng, room);
                if (!creature_can_gain_arena_experience(thing))
                {
                    clear_arena_partner(prtng);
                    finish_creature_arena_session(thing);
                    return CrStRet_Modified;
                }
            }
        }
        break;
    default:
        WARNLOG("Invalid %s arena mode %d; reset",thing_model_name(thing),(int)cctrl->training.mode);
        cctrl->training.mode = CrTrMd_SearchForTrainPost;
        cctrl->training.search_timeout = 0;
        break;
    }
    process_job_stress_and_going_postal(thing);
    SYNCDBG(18,"End");
    return CrStRet_Modified;
}

/**
 *  Finds a random training post near to the current position of given creature.
 *  Used when finding a training post seems to be taking too long; in that case, creature should start training with a nearest post.
 *  Note that this routine does not always select the nearest post - it is enough if it's 3 subtiles away.
 *
 * @param creatng The creature who wish to train with training post.
 */
void setup_training_search_for_post(struct Thing *creatng)
{
    struct Room* room = get_room_thing_is_on(creatng);
    // Let's start from a random slab
    long slb_x = -1;
    long slb_y = -1;
    long min_distance = INT32_MAX;
    struct Thing* traintng = INVALID_THING;
    long start_slab = THING_RANDOM(creatng, room->slabs_count);
    long k = start_slab;
    long i = room->slabs_list;
    while (i != 0)
    {
        slb_x = slb_num_decode_x(i);
        slb_y = slb_num_decode_y(i);
        i = get_next_slab_number_in_room(i);
        if (k <= 0)
            break;
        k--;
    }
    // Got random starting slab, now sweep room slabs from it
    struct Thing* thing = INVALID_THING;
    k = room->slabs_count;
    i = get_slab_number(slb_x,slb_y);
    while (k > 0)
    {
        slb_x = slb_num_decode_x(i);
        slb_y = slb_num_decode_y(i);
        i = get_next_slab_number_in_room(i);
        if (i == 0)
          i = room->slabs_list;
        // Per room tile code - find a nearest training post
        thing = get_object_at_subtile_of_model_and_owned_by(slab_subtile_center(slb_x), slab_subtile_center(slb_y), 31, creatng->owner);
        if (!thing_is_invalid(thing))
        {
            long dist = get_2d_distance(&creatng->mappos, &thing->mappos);
            if (dist < min_distance) {
                traintng = thing;
                min_distance = dist;
                if (min_distance < (3<<8))
                    break;
            }
        }
        // Per room tile code ends
        k--;
    }
    // Got trainer (or not...), now do the correct action
    if (thing_is_invalid(traintng))
    {
        SYNCDBG(6,"Room no longer have training post, moving somewhere else.");
        setup_move_to_new_training_position(creatng, room, true);
    } else
    {
        i = get_subtile_number(traintng->mappos.x.stl.num,traintng->mappos.y.stl.num);
        setup_training_move_near(creatng, i);
    }
}

struct Thing *find_training_post_just_next_to_creature(struct Thing *creatng)
{
    struct Thing* traintng = INVALID_THING;
    for (long i = 0; i < 4; i++)
    {
        long stl_x = creatng->mappos.x.stl.num + (long)small_around[i].delta_x;
        long stl_y = creatng->mappos.y.stl.num + (long)small_around[i].delta_y;
        traintng = get_object_at_subtile_of_model_and_owned_by(stl_x, stl_y, 31, creatng->owner);
        if (!thing_is_invalid(traintng))
            break;
    }
    return traintng;
}

void process_creature_in_training_room(struct Thing *thing, struct Room *room)
{
    static const struct Around corners[] = {
        {1, 2},
        {0, 1},
        {1, 0},
        {2, 1},
    };
    struct CreatureControl *cctrl;
    struct CreatureModelConfig *crconf;
    struct Thing *traintng;
    struct Thing *crtng;
    struct CreatureControl *cctrl2;
    struct Coord3d pos;
    long speed;
    long dist;
    long i;
    cctrl = creature_control_get_from_thing(thing);
    SYNCDBG(8,"Starting %s mode %d",thing_model_name(thing),(int)cctrl->training.mode);
    cctrl->annoy_untrained_turn = 0;
    switch (cctrl->training.mode)
    {
    case CrTrMd_SearchForTrainPost:
        // While we're in an instance, just wait
        if (cctrl->instance_id != CrInst_NULL)
            break;
        // On timeout, search for nearby training posts to start training ASAP
        if (cctrl->training.search_timeout < 1)
        {
            SYNCDBG(6,"Search timeout - selecting post nearest to (%d,%d)",(int)thing->mappos.x.stl.num, (int)thing->mappos.y.stl.num);
            setup_training_search_for_post(thing);
            cctrl->training.search_timeout = 100;
            break;
        }
        // Do a moving step
        cctrl->training.search_timeout--;
        speed = get_creature_speed(thing);
        i = creature_move_to(thing, &cctrl->moveto_pos, speed, 0, 0);
        if (i == 1)
        {
            // Move target is reached - find a training post which is supposed to be around here
            traintng = find_training_post_just_next_to_creature(thing);
            if (thing_is_invalid(traintng))
            {
                SYNCDBG(6,"Reached (%d,%d) but there's no training post there",(int)thing->mappos.x.stl.num, (int)thing->mappos.y.stl.num);
                setup_move_to_new_training_position(thing, room, false);
                break;
            }
            // Found - go to next mode
            cctrl->training.mode = CrTrMd_SelectPositionNearTrainPost;
            cctrl->training.search_timeout = 50;
        } else
        if (i == -1)
        {
            ERRORLOG("Cannot get to (%d,%d) in the training room",(int)cctrl->moveto_pos.x.stl.num,(int)cctrl->moveto_pos.y.stl.num);
            set_start_state(thing);
        }
        break;
    case CrTrMd_SelectPositionNearTrainPost:
        for (i=0; i < 4; i++)
        {
            long slb_x;
            long slb_y;
            long stl_x;
            long stl_y;
            struct SlabMap *slb;
            slb_x = subtile_slab(thing->mappos.x.stl.num) + (long)small_around[i].delta_x;
            slb_y = subtile_slab(thing->mappos.y.stl.num) + (long)small_around[i].delta_y;
            slb = get_slabmap_block(slb_x,slb_y);
            if ((slb->kind != SlbT_TRAINING) || (slabmap_owner(slb) != thing->owner))
                continue;
            stl_x = slab_subtile(slb_x,corners[i].delta_x);
            stl_y = slab_subtile(slb_y,corners[i].delta_y);
            traintng = INVALID_THING;
            // Check if any other creature is using that post; allow only unused posts
            crtng = get_creature_of_model_training_at_subtile_and_owned_by(stl_x, stl_y, -1, thing->owner, thing->index);
            if (thing_is_invalid(crtng))
            {
                traintng = get_object_at_subtile_of_model_and_owned_by(slab_subtile_center(slb_x), slab_subtile_center(slb_y), 31, thing->owner);
            }
            if (!thing_is_invalid(traintng))
            {
                cctrl->training.pole_stl_x = slab_subtile_center(subtile_slab(thing->mappos.x.stl.num));
                cctrl->training.pole_stl_y = slab_subtile_center(subtile_slab(thing->mappos.y.stl.num));
                cctrl->moveto_pos.x.stl.num = stl_x;
                cctrl->moveto_pos.y.stl.num = stl_y;
                cctrl->moveto_pos.x.stl.pos = 128;
                cctrl->moveto_pos.y.stl.pos = 128;
                cctrl->moveto_pos.z.val = get_thing_height_at(thing, &cctrl->moveto_pos);
                if (thing_in_wall_at(thing, &cctrl->moveto_pos))
                {
                    ERRORLOG("Illegal setup to (%d,%d)", (int)cctrl->moveto_pos.x.stl.num, (int)cctrl->moveto_pos.y.stl.num);
                    break;
                }
                cctrl->training.mode = CrTrMd_MoveToTrainPost;
                break;
            }
        }
        if (cctrl->training.mode == CrTrMd_SelectPositionNearTrainPost)
          setup_move_to_new_training_position(thing, room, 1);
        break;
    case CrTrMd_MoveToTrainPost:
        speed = get_creature_speed(thing);
        i = creature_move_to(thing, &cctrl->moveto_pos, speed, 0, 0);
        if (i == 1)
        {
            // If there's already someone training at that position, go somewhere else
            crtng = get_creature_of_model_training_at_subtile_and_owned_by(thing->mappos.x.stl.num, thing->mappos.y.stl.num, -1, thing->owner, thing->index);
            if (!thing_is_invalid(crtng))
            {
                setup_move_to_new_training_position(thing, room, 1);
                break;
            }
            // Otherwise, train at this position
            cctrl->training.mode = CrTrMd_TurnToTrainPost;
        } else
        if (i == -1)
        {
            ERRORLOG("Cannot get where we're going in the training room.");
            set_start_state(thing);
        }
        break;
    case CrTrMd_TurnToTrainPost:
        pos.x.val = subtile_coord_center(cctrl->training.pole_stl_x);
        pos.y.val = subtile_coord_center(cctrl->training.pole_stl_y);
        if (creature_turn_to_face(thing, &pos) < DEGREES_10)
        {
          cctrl->training.mode = CrTrMd_DoTrainWithTrainPost;
          cctrl->training.train_timeout = 75;
        }
        break;
    case CrTrMd_PartnerTraining:
        if (cctrl->training.partner_idx == 0)
        {
            setup_move_to_new_training_position(thing, room, false);
            return;
        }
        crtng = thing_get(cctrl->training.partner_idx);
        TRACE_THING(crtng);
        if (!thing_exists(crtng) || (get_creature_state_besides_move(crtng) != CrSt_Training) || (crtng->creation_turn != cctrl->training.partner_creation))
        {
            SYNCDBG(8,"The %s cannot start partner training - creature to train with is gone.",thing_model_name(thing));
            setup_move_to_new_training_position(thing, room, false);
            return;
        }
        cctrl2 = creature_control_get_from_thing(crtng);
        if (cctrl2->training.partner_idx != thing->index)
        {
            SYNCDBG(6,"The %s cannot start partner training - %s changed the partner.",thing_model_name(thing),thing_model_name(crtng));
            cctrl->training.partner_idx = 0;
            setup_move_to_new_training_position(thing, room, false);
            break;
        }
        if (get_room_thing_is_on(crtng) != room)
        {
            SYNCDBG(8,"The %s cannot start partner training - partner has left the room.",thing_model_name(thing));
            cctrl->training.partner_idx = 0;
            cctrl2->training.partner_idx = 0;
            setup_move_to_new_training_position(thing, room, false);
            break;
        }
        crconf = creature_stats_get_from_thing(thing);
        dist = get_combat_distance(thing, crtng);
        if (dist > 284)
        {
            if (creature_move_to(thing, &crtng->mappos, get_creature_speed(thing), 0, 0) == -1)
            {
              WARNLOG("The %s cannot navigate to training partner",thing_model_name(thing));
              setup_move_to_new_training_position(thing, room, false);
              cctrl->training.partner_idx = 0;
            }
        } else
        if (dist >= 156)
        {
            if (creature_turn_to_face(thing, &crtng->mappos) < DEGREES_10)
            {
              cctrl->training.train_timeout--;
              if (cctrl->training.train_timeout > 0)
              {
                if ((cctrl->instance_id == CrInst_NULL) && ((cctrl->training.train_timeout % 8) == 0))
                {
                    set_creature_instance(thing, CrInst_SWING_WEAPON_SWORD, 0, 0);
                }
              } else
              {
                if (cctrl->instance_id == CrInst_NULL)
                {
                    setup_move_to_new_training_position(thing, room, false);
                    cctrl->training.partner_idx = 0;
                } else
                {
                    cctrl->training.train_timeout = 1;
                }
                cctrl->exp_points += (room->efficiency * crconf->training_value);
              }
            }
        } else
        {
            creature_retreat_from_combat(thing, crtng, CrSt_Training, 0);
        }
        break;
    case CrTrMd_DoTrainWithTrainPost:
        if (cctrl->training.train_timeout > 0)
        {
            // While training timeout is positive, continue initiating the train instances
            cctrl->training.train_timeout--;
            if ((cctrl->instance_id == CrInst_NULL) && ((cctrl->training.train_timeout % 8) == 0))
            {
                set_creature_instance(thing, CrInst_SWING_WEAPON_SWORD, 0, 0);
            }
        } else
        {
            // Wait for the instance to end, then select new move position
            if (cctrl->instance_id != CrInst_NULL)
            {
                cctrl->training.train_timeout = 0;
            } else
            {
                cctrl->training.train_timeout = 0;
                setup_move_to_new_training_position(thing, room, true);
            }
        }
        break;
    default:
        WARNLOG("Invalid %s training mode %d; reset",thing_model_name(thing),(int)cctrl->training.mode);
        cctrl->training.mode = CrTrMd_SearchForTrainPost;
        cctrl->training.search_timeout = 0;
        break;
    }
    process_job_stress_and_going_postal(thing);
    SYNCDBG(18,"End");
}

short at_training_room(struct Thing *thing)
{
    struct CreatureControl* cctrl = creature_control_get_from_thing(thing);
    cctrl->target_room_id = 0;
    if (!creature_can_be_trained(thing))
    {
        SYNCDBG(9,"Ending training of %s level %d; creature is not trainable",thing_model_name(thing),(int)cctrl->exp_level);
        set_start_state(thing);
        return 0;
    }
    if (!player_can_afford_to_train_creature(thing))
    {
        if (is_my_player_number(thing->owner))
            output_message(SMsg_NoGoldToTrain, MESSAGE_DURATION_TREASURY);
        set_start_state(thing);
        return 0;
    }
    struct Room* room = get_room_thing_is_on(thing);
    if (!room_initially_valid_as_type_for_thing(room, get_room_role_for_job(Job_TRAIN), thing))
    {
        WARNLOG("Room %s owned by player %d is invalid for %s",room_code_name(room->kind),(int)room->owner,thing_model_name(thing));
        set_start_state(thing);
        return 0;
    }
    if (!add_creature_to_work_room(thing, room, Job_TRAIN))
    {
        set_start_state(thing);
        return 0;
    }
    internal_set_thing_state(thing, get_continue_state_for_job(Job_TRAIN));
    setup_move_to_new_training_position(thing, room, 1);
    cctrl->turns_at_job = 0;
    return 1;
}

CrStateRet training(struct Thing *thing)
{
    TRACE_THING(thing);
    SYNCDBG(18,"Starting");
    struct CreatureControl* cctrl = creature_control_get_from_thing(thing);
    // Check if we should finish training
    if (!creature_can_be_trained(thing))
    {
        SYNCDBG(9,"Ending training of %s level %d; creature is not trainable",thing_model_name(thing),(int)cctrl->exp_level);
        remove_creature_from_work_room(thing);
        set_start_state(thing);
        return CrStRet_ResetOk;
    }
    if (!player_can_afford_to_train_creature(thing))
    {
        SYNCDBG(19,"Ending training %s index %d; cannot afford",thing_model_name(thing),(int)thing->index);
        if (is_my_player_number(thing->owner))
            output_message(SMsg_NoGoldToTrain, MESSAGE_DURATION_TREASURY);
        remove_creature_from_work_room(thing);
        set_start_state(thing);
        return CrStRet_ResetFail;
    }
    // Check if we're in correct room
    struct Room* room = get_room_thing_is_on(thing);
    if (creature_job_in_room_no_longer_possible(room, Job_TRAIN, thing))
    {
        remove_creature_from_work_room(thing);
        set_start_state(thing);
        return CrStRet_ResetFail;
    }
    struct Dungeon* dungeon = get_dungeon(thing->owner);
    GoldAmount training_cost = calculate_correct_creature_training_cost(thing);
    // Pay for the training
    cctrl->turns_at_job++;
    if (cctrl->turns_at_job >= game.conf.rules[thing->owner].rooms.train_cost_frequency)
    {
        cctrl->turns_at_job -= game.conf.rules[thing->owner].rooms.train_cost_frequency;
        if (take_money_from_dungeon(thing->owner, training_cost, 1) < 0) {
            ERRORLOG("Cannot take %d gold from dungeon %d",(int)training_cost,(int)thing->owner);
        }
        create_price_effect(&thing->mappos, thing->owner, training_cost);
    }
    if ((cctrl->instance_id != CrInst_NULL) || !check_experience_upgrade(thing))
    {
        long work_value = compute_creature_work_value_for_room_role(thing, RoRoF_CrTrainExp, room->efficiency);
        SYNCDBG(19,"The %s index %d produced %d training points",thing_model_name(thing),(int)thing->index,(int)work_value);
        cctrl->exp_points += work_value;
        dungeon->total_experience_creatures_gained += work_value;
        process_creature_in_training_room(thing, room);
    } else
    {
        if (external_set_thing_state(thing, CrSt_CreatureBeHappy)) {
            cctrl->countdown = 50;
        }
        dungeon->lvstats.creatures_trained++;
    }
    return CrStRet_Modified;
}

short at_arena_room(struct Thing *thing)
{
    struct CreatureControl* cctrl = creature_control_get_from_thing(thing);
    cctrl->target_room_id = 0;
    if (!creature_can_do_arena(thing))
    {
        SYNCDBG(9,"Ending arena job of %s; creature cannot duel",thing_model_name(thing));
        set_start_state(thing);
        return 0;
    }
    struct Room* room = get_room_thing_is_on(thing);
    if (!room_initially_valid_as_type_for_thing(room, get_room_role_for_job(Job_ARENA), thing))
    {
        WARNLOG("Room %s owned by player %d is invalid for %s",room_code_name(room->kind),(int)room->owner,thing_model_name(thing));
        set_start_state(thing);
        return 0;
    }
    if (!add_creature_to_work_room(thing, room, Job_ARENA))
    {
        set_start_state(thing);
        return 0;
    }
    internal_set_thing_state(thing, get_continue_state_for_job(Job_ARENA));
    setup_move_to_new_arena_position(thing, room, true);
    cctrl->turns_at_job = 0;
    return 1;
}

CrStateRet arena_duel(struct Thing *thing)
{
    TRACE_THING(thing);
    SYNCDBG(18,"Starting");
    if (!creature_can_do_arena(thing))
    {
        SYNCDBG(9,"Ending arena job of %s; creature cannot duel",thing_model_name(thing));
        remove_creature_from_work_room(thing);
        set_start_state(thing);
        return CrStRet_ResetOk;
    }
    struct Room* room = get_room_thing_is_on(thing);
    if (creature_job_in_room_no_longer_possible(room, Job_ARENA, thing))
    {
        remove_creature_from_work_room(thing);
        set_start_state(thing);
        return CrStRet_ResetFail;
    }
    return process_creature_in_arena_room(thing, room);
}

/******************************************************************************/
