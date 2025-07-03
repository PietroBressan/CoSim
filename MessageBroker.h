//This class encapsulates all the logic behind the intra-satellite communication
//as well as the satellite - ground station communication

struct Message{
    int sender;
    int receiver;
    int orbitIndex;
    std::vector<Schedule> schedule;

    Message(int sender, int receiver, int orbitIndex, std::vector<Schedule> schedule):
    sender(sender),
    receiver(receiver),
    orbitIndex(orbitIndex),
    schedule(schedule){}
};

class MessageBroker{
    std::vector<Message> messages;

    void sendMessage(Message msg){
        messages.push_back(msg);
    }


    std::vector<Message> receiveMessages(int receiver, int orbitIndex){
        std::vector<Message> incomingMsg;
        for(size_type i = 0; i < messages.size(); i++) 
            if(messages[i].receiver == receiver && messages[i].orbitIndex == orbitIndex)
                incomingMsg.push_back(messages[i]);
        return incomingMsg;
    }

    public:

    void clear(){ if(!messages.empty()) messages.clear();}
};