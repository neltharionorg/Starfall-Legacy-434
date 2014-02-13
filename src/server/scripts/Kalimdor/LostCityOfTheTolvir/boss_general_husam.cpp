/*
 * Copyright (C) 2011-2013 NorthStrider
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include"lost_city_of_the_tolvir.h"
#include "SpellScript.h"

enum Spells
{
    SPELL_SUMMON_SHOCKWAVE_DUMMY_N      = 83131,
    SPELL_SUMMON_SHOCKWAVE_DUMMY_S      = 83132,
    SPELL_SUMMON_SHOCKWAVE_DUMMY_E      = 83133,
    SPELL_SUMMON_SHOCKWAVE_DUMMY_W      = 83134,
    SPELL_SUMMON_SHOCKWAVE_VISUAL       = 83128,
    SPELL_SHOCKWAVE                     = 83445,
    SPELL_SHOCKWAVE_MISSILE             = 83130,
    SPELL_SHOCKWAVE_DAMAGE              = 83454,
    SPELL_SHOCKWAVE_VISUAL_SUMMON       = 83129,
    SPELL_SUMMON_MYSTIC_TRAP            = 83644,
    SPELL_THROW_LANDMINES               = 83122, // Casted on each landmine
    SPELL_THROW_LAND_MINES_TARGET       = 83646,
    SPELL_DETONATE_TRAPS                = 91263,
    SPELL_BAD_INTENTIONS                = 83113,
    SPELL_THROW_PILLAR                  = 81350,
    SPELL_HARD_IMPACT                   = 83339,
    SPELL_HAMMER_FIST                   = 83654,

    // Traps
    SPELL_LAND_MINE_EXPLODE             = 83171,
    SPELL_LAND_MINE_VISUAL              = 83110,
    SPELL_LAND_MINE_SEARCH              = 83111,
    SPELL_LAND_MINE_SEARCH_TRIGGERED    = 83112,
    SPELL_LAND_MINE_ACTIVATE            = 85523,

    SPELL_RIDE_VEHICLE_HARDCODED        = 46598,
};

enum Texts
{
    SAY_AGGRO               = 0,
    SAY_SHOCKWAVE           = 1,
    SAY_ANNOUNCE_SHOCKWAVE  = 2,
    SAY_TRAP                = 3,
    SAY_DEATH               = 4,
    SAY_SLAY                = 5,
};

enum Events
{
    // General Husam
    EVENT_SHOCKWAVE = 1,
    EVENT_SUMMON_MYSTIC_TRAP,
    EVENT_SEARCH_MINES,
    EVENT_DETONATE_TRAPS,
    EVENT_BAD_INTENTIONS,
    EVENT_THROW_PLAYER,
    EVENT_DAMAGE_PLAYER,
    EVENT_HAMMER_FIST,

    // Shockwave Visual
    EVENT_SHOCKWAVE_DAMAGE,

    // Traps
    EVENT_PREPARE_TRAP,
    EVENT_SCHEDULE_EXPLODE,
    EVENT_EXPLODE,
    EVENT_SUMMON_TRAP,
};

enum Actions
{
    ACTION_DETONATE = 1,
};

class boss_general_husam : public CreatureScript
{
public:
    boss_general_husam() : CreatureScript("boss_general_husam") { }

    struct boss_general_husamAI : public BossAI
    {
        boss_general_husamAI(Creature* creature) : BossAI(creature, DATA_GENERAL_HUSAM)
        {
            triggerCount = 0;
            player = NULL;
        }

        uint8 triggerCount;
        Unit* player;

        void Reset()
        {
            _Reset();
            triggerCount = 0;
            player = NULL;
        }

        void EnterCombat(Unit* /*who*/)
        {
            _EnterCombat();
            Talk(SAY_AGGRO);
            instance->SendEncounterUnit(ENCOUNTER_FRAME_ENGAGE, me);
            events.ScheduleEvent(EVENT_SHOCKWAVE, 18000);
            events.ScheduleEvent(EVENT_SUMMON_MYSTIC_TRAP, 7500);
            events.ScheduleEvent(EVENT_BAD_INTENTIONS, 12000);
            events.ScheduleEvent(EVENT_HAMMER_FIST, 9800);
            if (IsHeroic())
                events.ScheduleEvent(EVENT_DETONATE_TRAPS, 26000);
        }

        void EnterEvadeMode()
        {
            _EnterEvadeMode();
            instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me);
            me->GetMotionMaster()->MoveTargetedHome();
            triggerCount = 0;
            Cleanup();
        }

        void KilledUnit(Unit* killed)
        {
            if (killed->GetTypeId() == TYPEID_PLAYER)
                Talk(SAY_SLAY);
        }

        void JustSummoned(Creature* summon)
        {
            switch (summon->GetEntry())
            {
                case NPC_SHOCKWAVE_TRIGGER:
                    triggerCount++;
                    if (triggerCount == 1)
                        summon->SetPosition(me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), me->GetOrientation());
                    else if (triggerCount == 2)
                        summon->SetOrientation(me->GetOrientation() + M_PI);
                    else if (triggerCount == 3)
                        summon->SetOrientation(me->GetOrientation() + M_PI/2);
                    else if (triggerCount == 4)
                        summon->SetOrientation(me->GetOrientation() - M_PI/2);

                    summon->GetMotionMaster()->MovePoint(0, summon->GetPositionX()+cos(summon->GetOrientation()) * 200, summon->GetPositionY()+sin(summon->GetOrientation()) * 200, summon->GetPositionZ(), false);
                    if (triggerCount > 3)
                    {
                        DoCastAOE(SPELL_SHOCKWAVE);
                        triggerCount = 0;
                    }
                    summons.Summon(summon);
                    break;
                default:
                    summons.Summon(summon);
                    break;

            }
        }

        void JustDied(Unit* /*who*/)
        {
            _JustDied();
            Talk(SAY_DEATH);
            instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me);
            Cleanup();
        }

        void Cleanup()
        {
            std::list<Creature*> units;
            GetCreatureListWithEntryInGrid(units, me, NPC_TOLVIR_LANDMINE_DUMMY, 500.0f);
            GetCreatureListWithEntryInGrid(units, me, NPC_TOLVIR_LANDMINE_PASSENGER, 500.0f);
            GetCreatureListWithEntryInGrid(units, me, NPC_TOLVIR_LANDMINE_VEHICLE, 500.0f);
            for (std::list<Creature*>::iterator itr = units.begin(); itr != units.end(); ++itr)
                (*itr)->DespawnOrUnsummon(1);

        }

        void UpdateAI(uint32 diff)
        {
            if (!UpdateVictim())
                return;

            events.Update(diff);

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_SHOCKWAVE:
                        Talk(SAY_SHOCKWAVE);
                        Talk(SAY_ANNOUNCE_SHOCKWAVE);
                        DoCastAOE(SPELL_SUMMON_SHOCKWAVE_DUMMY_N);
                        DoCastAOE(SPELL_SUMMON_SHOCKWAVE_DUMMY_S);
                        DoCastAOE(SPELL_SUMMON_SHOCKWAVE_DUMMY_E);
                        DoCastAOE(SPELL_SUMMON_SHOCKWAVE_DUMMY_W);
                        events.ScheduleEvent(EVENT_SHOCKWAVE, 39500);
                        break;
                    case EVENT_SUMMON_MYSTIC_TRAP:
                        DoCastAOE(SPELL_SUMMON_MYSTIC_TRAP);
                        events.ScheduleEvent(EVENT_SUMMON_MYSTIC_TRAP, 16500);
                        events.ScheduleEvent(EVENT_SEARCH_MINES, 300);
                        break;
                    case EVENT_SEARCH_MINES:
                    {
                        std::list<Creature*> units;
                        GetCreatureListWithEntryInGrid(units, me, NPC_TOLVIR_LANDMINE_DUMMY, 500.0f);
                        for (std::list<Creature*>::iterator itr = units.begin(); itr != units.end(); ++itr)
                            me->CastSpell(*itr, SPELL_THROW_LANDMINES);
                        break;
                    }
                    case EVENT_BAD_INTENTIONS:
                        if (player = SelectTarget(SELECT_TARGET_RANDOM, 0, 100.0f, true))
                        {
                            DoCast(player, SPELL_BAD_INTENTIONS);
                            player->CastSpell(me, SPELL_RIDE_VEHICLE_HARDCODED);
                            events.ScheduleEvent(EVENT_THROW_PLAYER, 500);
                        }
                        events.ScheduleEvent(EVENT_BAD_INTENTIONS, 25000);
                        break;
                    case EVENT_THROW_PLAYER:
                        if (Creature* dummy = me->FindNearestCreature(NPC_BAD_INTENTIONS_TARGET, 200.0f))
                        {
                            me->RemoveAurasDueToSpell(SPELL_RIDE_VEHICLE_HARDCODED);
                            DoCastAOE(SPELL_THROW_PILLAR);
                            player->GetMotionMaster()->MoveJump(dummy->GetPositionX(), dummy->GetPositionY(), dummy->GetPositionZ(), 50.0f, 5.0f);
                            events.ScheduleEvent(EVENT_DAMAGE_PLAYER, 1000);
                        }
                        break;
                    case EVENT_DAMAGE_PLAYER:
                        DoCast(player, SPELL_HARD_IMPACT);
                        break;
                    case EVENT_DETONATE_TRAPS:
                        DoCastAOE(SPELL_DETONATE_TRAPS);
                        events.ScheduleEvent(EVENT_DETONATE_TRAPS, 90000);
                        break;
                    case EVENT_HAMMER_FIST:
                        DoCastAOE(SPELL_HAMMER_FIST);
                        events.ScheduleEvent(EVENT_HAMMER_FIST, 20000);
                        break;
                    default:
                        break;
                }
            }

            DoMeleeAttackIfReady();
        }
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new boss_general_husamAI(creature);
    }
};

class npc_lct_shockwave_visual : public CreatureScript
{
    public:
        npc_lct_shockwave_visual() :  CreatureScript("npc_lct_shockwave_visual") { }

        struct npc_lct_shockwave_visualAI : public ScriptedAI
        {
            npc_lct_shockwave_visualAI(Creature* creature) : ScriptedAI(creature)
            {
            }

            EventMap events;

            void IsSummonedBy(Unit* /*summoner*/)
            {
                events.ScheduleEvent(EVENT_SHOCKWAVE_DAMAGE, 4500);
                me->SetReactState(REACT_PASSIVE);
                SetCombatMovement(false);
            }

            void UpdateAI(uint32 diff)
            {
                events.Update(diff);

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_SHOCKWAVE_DAMAGE:
                            DoCastAOE(SPELL_SHOCKWAVE_DAMAGE);
                            me->DespawnOrUnsummon(500);
                            break;
                        default:
                            break;
                    }
                }
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_lct_shockwave_visualAI(creature);
        }
};

class npc_lct_landmine_vehicle : public CreatureScript
{
    public:
        npc_lct_landmine_vehicle() :  CreatureScript("npc_lct_landmine_vehicle") { }

        struct npc_lct_landmine_vehicleAI : public ScriptedAI
        {
            npc_lct_landmine_vehicleAI(Creature* creature) : ScriptedAI(creature)
            {
            }

            EventMap events;

            void IsSummonedBy(Unit* /*summoner*/)
            {
                me->SetReactState(REACT_PASSIVE);
                SetCombatMovement(false);
                events.ScheduleEvent(EVENT_SUMMON_TRAP, 1000);
            }

            void UpdateAI(uint32 diff)
            {
                events.Update(diff);

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_SUMMON_TRAP:
                            if (Creature* mine = me->SummonCreature(NPC_TOLVIR_LANDMINE_PASSENGER, me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), me->GetOrientation(), TEMPSUMMON_MANUAL_DESPAWN))
                                mine->AI()->DoCast(me, SPELL_RIDE_VEHICLE_HARDCODED);
                            break;
                        default:
                            break;
                    }
                }
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_lct_landmine_vehicleAI(creature);
        }
};

class npc_lct_landmine_passenger : public CreatureScript
{
    public:
        npc_lct_landmine_passenger() :  CreatureScript("npc_lct_landmine_passenger") { }

        struct npc_lct_landmine_passengerAI : public ScriptedAI
        {
            npc_lct_landmine_passengerAI(Creature* creature) : ScriptedAI(creature)
            {
            }

            EventMap events;

            void IsSummonedBy(Unit* /*summoner*/)
            {
                me->SetReactState(REACT_PASSIVE);
                SetCombatMovement(false);
                events.ScheduleEvent(EVENT_PREPARE_TRAP, 2000);
            }

            void DoAction(int32 action)
            {
                switch (action)
                {
                    case ACTION_DETONATE:
                        me->RemoveAurasDueToSpell(SPELL_LAND_MINE_SEARCH);
                        me->RemoveAurasDueToSpell(SPELL_LAND_MINE_VISUAL);
                        DoCastAOE(SPELL_LAND_MINE_ACTIVATE);
                        events.ScheduleEvent(EVENT_EXPLODE, 10000);
                        break;
                }
            }

            void UpdateAI(uint32 diff)
            {
                events.Update(diff);

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_PREPARE_TRAP:
                            DoCastAOE(SPELL_LAND_MINE_SEARCH);
                            DoCastAOE(SPELL_LAND_MINE_VISUAL);
                            break;
                        case EVENT_EXPLODE:
                            me->DespawnCreaturesInArea(NPC_TOLVIR_LANDMINE_VEHICLE, 1.0f);
                            me->DespawnOrUnsummon(100);
                            break;
                        default:
                            break;
                    }
                }
            }

            void SpellHitTarget(Unit* target, SpellInfo const* spell)
            {
                if (spell->Id == SPELL_LAND_MINE_SEARCH_TRIGGERED && target->GetTypeId() == TYPEID_PLAYER)
                {
                    me->RemoveAurasDueToSpell(SPELL_LAND_MINE_SEARCH);
                    me->RemoveAurasDueToSpell(SPELL_LAND_MINE_VISUAL);
                    DoCastAOE(SPELL_LAND_MINE_ACTIVATE);
                    events.ScheduleEvent(EVENT_EXPLODE, 10000);
                }
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_lct_landmine_passengerAI(creature);
        }
};

class spell_lct_shockwave_summon : public SpellScriptLoader
{
    public:
        spell_lct_shockwave_summon() : SpellScriptLoader("spell_lct_shockwave_visual_summon") { }

        class spell_lct_shockwave_summon_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_lct_shockwave_summon_AuraScript);

            void OnPeriodic(AuraEffect const* /*aurEff*/)
            {
                GetCaster()->CastSpell(GetCaster(), SPELL_SUMMON_SHOCKWAVE_VISUAL);
            }

            void Register()
            {
                OnEffectPeriodic += AuraEffectPeriodicFn(spell_lct_shockwave_summon_AuraScript::OnPeriodic, EFFECT_0, SPELL_AURA_PERIODIC_TRIGGER_SPELL);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_lct_shockwave_summon_AuraScript();
        }
};

class spell_lct_shockwave : public SpellScriptLoader
{
    public:
        spell_lct_shockwave() : SpellScriptLoader("spell_lct_shockwave") { }

        class spell_lct_shockwave_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_lct_shockwave_SpellScript);

            void EffectScriptEffect(SpellEffIndex /*effIndex*/)
            {
                GetCaster()->CastSpell(GetHitUnit(), SPELL_SHOCKWAVE_MISSILE, true);
            }

            void Register()
            {
                OnEffectHitTarget += SpellEffectFn(spell_lct_shockwave_SpellScript::EffectScriptEffect, EFFECT_0, SPELL_EFFECT_SCRIPT_EFFECT);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_lct_shockwave_SpellScript();
        }
};

class spell_lct_bad_intentions : public SpellScriptLoader
{
public:
    spell_lct_bad_intentions() : SpellScriptLoader("spell_lct_bad_intentions") { }

    class spell_lct_bad_intentions_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_lct_bad_intentions_SpellScript);

        void HandleScriptEffect(SpellEffIndex /*effIndex*/)
        {
            GetHitUnit()->CastSpell(GetCaster(), SPELL_RIDE_VEHICLE_HARDCODED);
        }

        void Register()
        {
            OnEffectHitTarget += SpellEffectFn(spell_lct_bad_intentions_SpellScript::HandleScriptEffect, EFFECT_1, SPELL_EFFECT_SCRIPT_EFFECT);
        }
    };

    SpellScript* GetSpellScript() const
    {
        return new spell_lct_bad_intentions_SpellScript();
    }
};

class spell_lct_detonate_traps : public SpellScriptLoader
{
    public:
        spell_lct_detonate_traps() : SpellScriptLoader("spell_lct_detonate_traps") { }

        class spell_lct_detonate_traps_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_lct_detonate_traps_SpellScript);

            void EffectScriptEffect(SpellEffIndex /*effIndex*/)
            {
                GetHitUnit()->ToCreature()->AI()->DoAction(ACTION_DETONATE);
            }

            void Register()
            {
                OnEffectHitTarget += SpellEffectFn(spell_lct_detonate_traps_SpellScript::EffectScriptEffect, EFFECT_0, SPELL_EFFECT_SCRIPT_EFFECT);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_lct_detonate_traps_SpellScript();
        }
};

class spell_lct_hammer_fist : public SpellScriptLoader
{
    public:
        spell_lct_hammer_fist() : SpellScriptLoader("spell_lct_hammer_fist") { }

    private:
        class spell_lct_hammer_fist_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_lct_hammer_fist_AuraScript)

            void HandleTick(AuraEffect const* /*aurEff*/)
            {
                PreventDefaultAction();
                GetCaster()->CastSpell(GetCaster()->getVictim(), GetSpellInfo()->Effects[EFFECT_0].TriggerSpell, true);
            }

            void Register()
            {
                OnEffectPeriodic += AuraEffectPeriodicFn(spell_lct_hammer_fist_AuraScript::HandleTick, EFFECT_0, SPELL_AURA_PERIODIC_TRIGGER_SPELL);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_lct_hammer_fist_AuraScript();
        }
};

void AddSC_boss_general_husam()
{
    new boss_general_husam();
    new npc_lct_shockwave_visual();
    new npc_lct_landmine_vehicle();
    new npc_lct_landmine_passenger();
    new spell_lct_shockwave_summon();
    new spell_lct_shockwave();
    new spell_lct_bad_intentions();
    new spell_lct_detonate_traps();
    new spell_lct_hammer_fist();
}
