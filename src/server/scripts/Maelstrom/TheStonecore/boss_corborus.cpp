
#include "ScriptPCH.h"
#include "the_stonecore.h"

enum Spells
{
    // Intro
    SPELL_RING_WYRM_CHARGE              = 81237,
    SPELL_RING_WYRM_KNOCKBACK           = 81235,
    
    // Encounter
    SPELL_CRYSTAL_BARRAGE               = 81638,
    SPELL_CRYSTAL_BARRAGE_AREA_AURA     = 86881,
    SPELL_CRYSTAL_BARRAGE_SCRIPT        = 81634,
    SPELL_CRYSTAL_SHARD_SUMMON          = 92012, // After Barrage cast

    SPELL_DAMPENING_WAVE                = 82415,
    SPELL_CLEAR_ALL_DEBUFFS             = 34098,
    SPELL_SUBMERGE                      = 81629,
    SPELL_EMERGE                        = 81948,

    SPELL_THRASHING_CHARGE_TELEPORT     = 81839,
    SPELL_THRASHING_CHARGE_SUMMON       = 81816,
    SPELL_THRASHING_CHARGE_VISUAL       = 81801,
    SPELL_THRASHING_CHARGE_DAMAGE       = 81828,

    // Rock Borer
    SPELL_EMERGE_BEATLE                 = 82185,
    SPELL_ROCK_BORE                     = 80028,

    // Crystal Shards
    SPELL_CRYSTAL_SHARDS                = 92117,
    SPELL_RANDOM_AGGRO_TAUNT            = 92111,
    SPELL_CRYSTAL_SHARD_EXPLOSION       = 80913,
};

enum Events
{
    EVENT_INTRO = 1,
    EVENT_KNOCKBACK,
    EVENT_CRYSTAL_BARRAGE,
    EVENT_SUMMON_SHARD,
    EVENT_DAMPENING_WAVE,
    EVENT_SUBMERGE,
    EVENT_REMOVE_TARGET,
    EVENT_TRASHING_CHARGE_TELEPORT,
    EVENT_TRASHING_CHARGE_PREPARE,
    EVENT_EMERGE,
    EVENT_ATTACK,

    EVENT_ROCK_BORE,
};

enum Actions
{
    ACTION_INTRO = 1,
};

enum Phases
{
    PHASE_INTRO = 1,
    PHASE_BATTLE = 2,
};

Position const HomePos = {1154.55f, 878.843f, 286.0f, 3.2216f};

class boss_corborus : public CreatureScript
{
public:
    boss_corborus() : CreatureScript("boss_corborus") { }

    struct boss_corborusAI : public BossAI
    {
        boss_corborusAI(Creature* creature) : BossAI(creature, DATA_CORBORUS)
        {
        }

        void Reset()
        {
            _Reset();
        }

        void EnterCombat(Unit* who)
        {
            _EnterCombat();
            instance->SendEncounterUnit(ENCOUNTER_FRAME_ENGAGE, me);
            events.ScheduleEvent(EVENT_DAMPENING_WAVE, 11000);
            events.ScheduleEvent(EVENT_SUBMERGE, 31000);
        }

        void EnterEvadeMode()
        {
            _EnterEvadeMode();
            instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me);
            me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_IMMUNE_TO_NPC | UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_NOT_SELECTABLE);
            me->GetMotionMaster()->MoveTargetedHome();
            me->SetReactState(REACT_AGGRESSIVE);
            me->DespawnCreaturesInArea(NPC_ROCK_BORER, 500.0f);
            me->DespawnCreaturesInArea(NPC_CRYSTAL_SHARD, 500.0f);
        }

        void JustDied(Unit* /*killer*/)
        {
            _JustDied();
            instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me);
            me->DespawnCreaturesInArea(NPC_ROCK_BORER, 500.0f);
            me->DespawnCreaturesInArea(NPC_CRYSTAL_SHARD, 500.0f);
        }

        void UpdateAI(uint32 diff)
        {
            if (!(events.IsInPhase(PHASE_INTRO)))
                if (!UpdateVictim())
                    return;

            events.Update(diff);

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_INTRO:
                        if (GameObject* wall = me->FindNearestGameObject(GO_ROCKDOOR, 300.0f))
                            wall->SetGoState(GO_STATE_ACTIVE);
                        me->SetHomePosition(HomePos);
                        DoCastAOE(SPELL_RING_WYRM_CHARGE);
                        events.ScheduleEvent(EVENT_KNOCKBACK, 700);
                        break;
                    case EVENT_KNOCKBACK:
                    {
                        std::list<Creature*> units;
                        GetCreatureListWithEntryInGrid(units, me, NPC_STONECORE_BERSERKER, 200.0f);
                        GetCreatureListWithEntryInGrid(units, me, NPC_STONECORE_EARTHSHAPER, 200.0f);
                        GetCreatureListWithEntryInGrid(units, me, NPC_STONECORE_WARBRINGER, 200.0f);
                        GetCreatureListWithEntryInGrid(units, me, NPC_MILLHOUSE_MANASTORM, 200.0f);
                        for (std::list<Creature*>::iterator itr = units.begin(); itr != units.end(); ++itr)
                        {
                            (*itr)->CastStop();
                            (*itr)->DespawnOrUnsummon(3000);
                            (*itr)->AI()->DoCastAOE(SPELL_RING_WYRM_KNOCKBACK);
                        }
                        events.SetPhase(PHASE_BATTLE);
                        EnterEvadeMode();
                        break;
                    }
                    case EVENT_CRYSTAL_BARRAGE:
                        if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 0, true))
                            DoCast(target, SPELL_CRYSTAL_BARRAGE_AREA_AURA);
                        if (IsHeroic())
                            events.ScheduleEvent(EVENT_SUMMON_SHARD, 500);
                        break;
                    case EVENT_SUMMON_SHARD:
                        if (me->HasAura(SPELL_CRYSTAL_BARRAGE))
                        {
                            DoCastAOE(SPELL_CRYSTAL_SHARD_SUMMON);
                            events.ScheduleEvent(EVENT_SUMMON_SHARD, 500);
                        }
                        break;
                    case EVENT_DAMPENING_WAVE:
                        DoCastAOE(SPELL_DAMPENING_WAVE);
                        events.ScheduleEvent(EVENT_DAMPENING_WAVE, 11000);
                        events.ScheduleEvent(EVENT_CRYSTAL_BARRAGE, 1000);
                        break;
                    case EVENT_SUBMERGE:
                        DoCastAOE(SPELL_CLEAR_ALL_DEBUFFS);
                        DoCastAOE(SPELL_SUBMERGE);
                        events.CancelEvent(EVENT_DAMPENING_WAVE);
                        events.CancelEvent(EVENT_CRYSTAL_BARRAGE);
                        events.CancelEvent(EVENT_SUMMON_SHARD);
                        events.ScheduleEvent(EVENT_REMOVE_TARGET, 850);
                        events.ScheduleEvent(EVENT_TRASHING_CHARGE_TELEPORT, 1200);
                        events.ScheduleEvent(EVENT_EMERGE, 27000);
                        break;
                    case EVENT_REMOVE_TARGET:
                        me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_IMMUNE_TO_NPC | UNIT_FLAG_NOT_SELECTABLE);
                        me->AttackStop();
                        me->SetReactState(REACT_PASSIVE);
                        break;
                    case EVENT_TRASHING_CHARGE_TELEPORT:
                        if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 0, true))
                            DoCast(target, SPELL_THRASHING_CHARGE_TELEPORT);
                        events.ScheduleEvent(EVENT_TRASHING_CHARGE_PREPARE, 1200);
                        events.ScheduleEvent(EVENT_TRASHING_CHARGE_TELEPORT, 4800);
                        break;
                    case EVENT_TRASHING_CHARGE_PREPARE:
                        DoCastAOE(SPELL_THRASHING_CHARGE_SUMMON);
                        break;
                    case EVENT_EMERGE:
                        events.CancelEvent(EVENT_TRASHING_CHARGE_PREPARE);
                        events.CancelEvent(EVENT_TRASHING_CHARGE_TELEPORT);
                        me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_IMMUNE_TO_NPC | UNIT_FLAG_NOT_SELECTABLE);
                        me->CastStop();
                        me->RemoveAurasDueToSpell(SPELL_SUBMERGE);
                        DoCastAOE(SPELL_EMERGE);
                        events.ScheduleEvent(EVENT_ATTACK, 2500);
                        break;
                    case EVENT_ATTACK:
                        me->SetReactState(REACT_AGGRESSIVE);
                        events.ScheduleEvent(EVENT_DAMPENING_WAVE, 11000);
                        break;
                    default:
                        break;
                }
            }

            DoMeleeAttackIfReady();
        }

        void DoAction(int32 action)
        {
            switch (action)
            {
                case ACTION_INTRO:
                    events.SetPhase(PHASE_INTRO);
                    events.ScheduleEvent(EVENT_INTRO, 2600);
                    break;
            }
        }

        void JustSummoned(Creature* summon)
        {
            switch (summon->GetEntry())
            {
                case NPC_TRASHING_CHARGE:
                    summon->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_IMMUNE_TO_NPC | UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_IMMUNE_TO_PC);
                    summon->setFaction(16);
                    summon->SetFacingToObject(me);
                    me->SetFacingToObject(summon);
                    summon->AI()->DoCastAOE(SPELL_THRASHING_CHARGE_DAMAGE);
                    DoCastAOE(SPELL_THRASHING_CHARGE_VISUAL);
                    break;
            }
        }
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new boss_corborusAI (creature);
    }
};

class npc_tsc_rock_borer : public CreatureScript
{
public:
    npc_tsc_rock_borer() : CreatureScript("npc_tsc_rock_borer") { }

    struct npc_tsc_rock_borerAI : public ScriptedAI
    {
        npc_tsc_rock_borerAI(Creature* creature) : ScriptedAI(creature) 
        {
        }

        EventMap events;

        void IsSummonedBy(Unit* /*summoner*/)
        {
            me->SetReactState(REACT_PASSIVE);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_NPC | UNIT_FLAG_DISABLE_MOVE);
            events.ScheduleEvent(EVENT_ATTACK, 2500);
            DoCastAOE(SPELL_EMERGE_BEATLE);
        }

        void JustDied(Unit* /*killer*/)
        {
            me->DespawnOrUnsummon(3000);
        }

        void UpdateAI(uint32 diff)
        {
            events.Update(diff);

            while(uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_ATTACK:
                        me->SetReactState(REACT_AGGRESSIVE);
                        me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_NPC | UNIT_FLAG_DISABLE_MOVE);
                        if (Player* player = me->FindNearestPlayer(100.0f, true))
                            me->SetInCombatWithZone();
                        events.ScheduleEvent(EVENT_ROCK_BORE, 5000);
                        break;
                    case EVENT_ROCK_BORE:
                        DoCastVictim(SPELL_ROCK_BORE);
                        events.ScheduleEvent(EVENT_ROCK_BORE, 5000);
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
        return new npc_tsc_rock_borerAI(creature);
    }
};

void AddSC_boss_corborus()
{
    new boss_corborus();
    new npc_tsc_rock_borer();
}
