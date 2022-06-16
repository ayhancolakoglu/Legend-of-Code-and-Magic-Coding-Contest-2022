#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <limits.h>
#include <cstring>

using namespace std;

static const int MAX_MANA = 12;

void trap() {
    *(int *) 0 = 0;
}


struct Player {
    int healthp;
    int mana;
    int cardsOutstanding;
    int rune;
};

enum class CardLocation {
    InHand = 0,
    MyField = 1,
    OpField = -1
};

enum class CardType {
    Creature,
    Green,
    Red,
    Blue
};

struct Card {
    CardLocation location;
    CardType cardType;

    int id;
    int cardId;
    int cost;
    int attack;
    int defense;
    int healthPchange;
    int healthPchangeEnemy;
    int cardDraw;

    bool breakthrough;
    bool charge;
    bool guard;
};


struct State {
    Player players[2];
    int opHand;
    vector<Card> cards;

    bool isInDraft();

};

bool State::isInDraft() {
    return players[0].mana == 0;
}

//different game action types
enum class ActionType {
    Pass,
    Summon,
    Attack,
    Use,
    Pick
};

struct Action {
    ActionType type;
    int id = -1;
    int idTarget = -1;

    void pass();

    void summon(int id);

    void attack(int id, int targetId = -1);

    void use(int id, int targetId = -1);

    void pick(int id);

    void print(ostream &os);
};

void Action::pass() {
    type = ActionType::Summon;
}

void Action::summon(int _id) {
    type = ActionType::Summon;
    id = _id;
}

void Action::attack(int _id, int _targetId) {
    type = ActionType::Attack;
    id = _id;
    idTarget = _targetId;
}

void Action::use(int _id, int _targetId) {
    type = ActionType::Use;
    id = _id;
    idTarget = _targetId;
}


void Action::pick(int _id) {
    type = ActionType::Pick;
    id = _id;
}

//überladene ausgabe für verschiedene game actions
void Action::print(ostream &os) {
    if (type == ActionType::Pass) {
        os << "PASS";
    } else if (type == ActionType::Summon) {
        os << "SUMMON " << id;
    } else if (type == ActionType::Attack) {
        os << "ATTACK " << id << " " << idTarget;
    } else if (type == ActionType::Use) {
        os << "USE " << id << " " << idTarget;

    } else if (type == ActionType::Pick) {
        os << "PICK " << id;
    } else {
        cerr << "Action not found: " << (int) type << endl;
        trap();
    }
}


// ATTACK 2 4; ATTACK 1 2;

//Ein Struct für jede Runde um mehrere actions auszuführen und am ende zu clearen
struct Turn {
    vector<Action> actions;

    Action &newAction();

    void print(ostream &os);

    bool isCardPlayed(int id);

    void clear() { actions.clear(); }
};

Action &Turn::newAction() {
    actions.emplace_back();
    return actions.back();
}

bool Turn::isCardPlayed(int id) {
    for (Action &action: actions) {
        if (!(action.type == ActionType::Summon || action.type == ActionType::Use)) continue;
        if (action.id == id) return true;
    }
    return false;
}

//print so überladen, dass mehrere actions ausgeführt werden können
void Turn::print(ostream &os) {
    if (actions.size() == 0) {
        os << "PASS";
        return;
    }

    bool first = true;
    for (Action &action: actions) {
        if (!first) os << ";";
        first = false;
        action.print(os);
    }

}


struct ManaDiagram {
    int curve[MAX_MANA + 1];

    int evalScore();

    void compute(vector<Card> &cards);
    void print(ostream &os);
};

int ManaDiagram::evalScore() {

    int lowCount = 0, medCount = 0, hiCount = 0;
    for (int i = 0; i <= 2 ; i++) {
        lowCount += curve[i];
    }

    for (int i = 3; i <=5; i++) {
        medCount += curve[i];
    }
    for (int i = 6; i <= 8; i++) {
        hiCount += curve[i];
    }
    return (abs(lowCount -10) + abs(medCount -15) + abs(hiCount -5))*5;
}

void ManaDiagram::compute(vector<Card> &cards) {
    memset(curve, 0, sizeof(curve));
    for (Card &card: cards) {
        curve[card.cost]++;
    }
}

void ManaDiagram::print(ostream &os) {
    for (int i = 0; i < MAX_MANA ;  i++) {
        os << "Mana " << i << " = " << curve[i] << " cards" << endl;
    }
}
//Agent / Bot
struct Agent {
    State state;
    Turn bestTurn;
    vector<Card> draftedCards;

    void read();

    void think();

    void print();
};

void Agent::print() {
    bestTurn.print(cout);
    cout << endl;
}

void Agent::think() {
    bestTurn.clear();

    ManaDiagram curve;
    curve.compute(draftedCards);

    if (state.isInDraft()) {
        int bestScore = INT_MAX;
        int bestPick = -1;
        for (int i = 0; i < 3 ; i++) {
            Card &card = state.cards[i];
            curve.curve[card.cost]++;
            int score = curve.evalScore();
            curve.curve[card.cost] --;

            if(score < bestScore){
                bestScore = score;
                bestPick = i;
            }
        }


        //Schaut wie viele karten es  mit jeweiligen Mana gibt z.B Mana Kosten 4 = 3 Karten

        auto &action = bestTurn.newAction();
        action.pick(bestPick);
        draftedCards.push_back(state.cards[bestPick]);
        return;
    }
    curve.print(cerr);


    //Battle Phase
    //befüllung des vektors action mit verschiedenen game actions

    //einteilen der Karten (Creatures) Gegner und eigene Creatures in Vektoren
    vector<Card *> myCreatures;
    vector<Card *> otherCreatures;
    for (Card &card: state.cards) {
        if (card.location == CardLocation::MyField) {
            myCreatures.push_back(&card);
        } else if (card.location == CardLocation::OpField) {
            otherCreatures.push_back(&card);
        }
    }

    int myMana = state.players[0].mana;

    while (myMana > 0) {
        Card *bestCard = nullptr;
        int bestScore = -INT_MAX;

        for (Card &card: state.cards) {
            if (card.location != CardLocation::InHand) continue;
            if (card.cost > myMana) continue;
            if (card.cardType == CardType::Green && myCreatures.size() == 0) continue;
            if (card.cardType == CardType::Red && otherCreatures.size() == 0) continue;
            if (card.cardType == CardType::Blue) continue;
            if (bestTurn.isCardPlayed(card.id)) continue;


            //Searching for the card with highest mana
            int score = card.cost;
            if (score > bestScore) {
                bestScore = score;
                bestCard = &card;
            }
        }

        //"beste Karte" die hingelegt werden soll definieren
        if (bestCard == nullptr) { break; }

        bestTurn.actions.emplace_back();
        Action &action = bestTurn.actions.back();
        if (bestCard->cardType == CardType::Creature) {
            action.summon(bestCard->id);
            myCreatures.push_back(bestCard);
        } else {
            if (bestCard->cardType == CardType::Green) {
                action.use(bestCard->id, myCreatures.back()->id);
            } else if (bestCard->cardType == CardType::Red) {
                action.use(bestCard->id, otherCreatures.back()->id);
            }
        }


        myMana -= bestCard->cost;
    }

    //guckt ob die karte vom GEGNER auf dem Spielfeld guard ist oder nicht
    // und speichert sie dann im vector gurads ab falls es eine ist
    vector<Card *> guards;
    for (Card &card: state.cards) {
        if (card.location != CardLocation::OpField) { continue; }
        if (!card.guard) { continue; }
        guards.push_back(&card);
    }


    for (Card &card: state.cards) {
        if (card.location != CardLocation::MyField) { continue; }

        bestTurn.actions.emplace_back();
        Action &action = bestTurn.actions.back();

        //hier wird bestimmt welche karte welche karte angreift
        if (guards.size() == 0) { action.attack(card.id); }
        else {
            //falls gegner guard hat greift guard an
            auto guard = guards.back();
            guard->defense -= card.attack;
            action.attack(card.id, guard->id);

            if (guard->defense <= 0) { guards.pop_back(); }
        }
    }
}

void Agent::read() {
    //player eingelesen
    for (int i = 0; i < 2; i++) {
        Player &player = state.players[i];
        int player_health, player_mana, player_deck, player_rune, player_draw;
        cin >> player_health >> player_mana >> player_deck >> player_rune >> player_draw;
        cin.ignore();
        player.healthp = player_health;
        player.mana = player_mana;
        player.cardsOutstanding = player_deck;
        player.rune = player_rune;
    }

    int opponent_hand;
    int opponent_actions;
    cin >> opponent_hand >> opponent_actions;
    cin.ignore();
    state.opHand = opponent_hand;


    ///////////WHAT
    for (int i = 0; i < opponent_actions; i++) {
        string card_number_and_action;
        getline(cin, card_number_and_action);
    }
    /////////////

    int card_count;
    cin >> card_count;
    cin.ignore();
    state.cards.clear();
    for (int i = 0; i < card_count; i++) {

        state.cards.emplace_back();
        Card &card = state.cards.back();
        int card_number, instance_id, location, card_type, cost, attack, defense;
        string abilities;
        int my_health_change, opponent_health_change, card_draw;
        cin >> card_number >> instance_id >> location >> card_type >> cost >> attack >> defense >> abilities
            >> my_health_change >> opponent_health_change >> card_draw;
        cin.ignore();
        card.cardId = card_number;
        card.id = instance_id;
        card.location = (CardLocation) location;
        card.cardType = (CardType) card_type;
        card.cost = cost;
        card.attack = attack;
        card.defense = defense;
        card.healthPchange = my_health_change;
        card.healthPchangeEnemy = opponent_health_change;
        card.cardDraw = card_draw;

        for (char c: abilities) {
            if (c == 'B') {
                card.breakthrough = true;
            }
            if (c == 'C') {
                card.charge = true;
            }
            if (c == 'G') {
                card.guard = true;
            }
        }
    }
}

int main() {
    //AGENT COULD RUN HERE
    Agent agent;
    while (1) {
        agent.read();
        agent.think();
        agent.print();
    }
}
