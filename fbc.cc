#include <iostream>
#include <memory>
#include <thread>
#include <vector>
#include <string>
#include <unistd.h>
#include <vector>
#include <grpc++/grpc++.h>

#include "tsm.grpc.pb.h"

#include "client.h"

#include "command_assist.h"


using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using grpc::ClientReader;
using grpc::ClientReaderWriter;
using grpc::ClientWriter;

using tsm::UserName;
using tsm::UserId;
using tsm::TinySocialMedia;
using tsm::GeneralStatus;
using tsm::FollowCommand;
using tsm::UnfollowCommand;
using tsm::ListReply;
using tsm::ClientPost;
using tsm::ServerPost;





class Client : public IClient
{
    public:
        Client(const std::string& hname,
               const std::string& uname,
               const std::string& p)
            :hostname(hname), username(uname), port(p)
            {}
    protected:
        virtual int connectTo();
        virtual IReply processCommand(std::string& input);
        virtual void processTimeline();
    private:
        IReply processFollow(std::string& input);
        IReply processUnfollow(std::string& input);
        IReply processList(std::string& input);
        IReply processTimeline(std::string& input);

        std::string hostname;
        std::string username;
        std::string port;
        
        // You can have an instance of the client stub
        // as a member variable.
        std::unique_ptr<TinySocialMedia::Stub> stub_;

        // Context for the client. It could be used to convey extra information to
        // the server and/or tweak certain RPC behaviors.
        ClientContext context_;

        int64_t myId_;
};

int main(int argc, char** argv) {
    std::string hostname = "localhost";
    std::string username = "default";
    std::string port = "3010";


    int opt = 0;
    while ((opt = getopt(argc, argv, "h:u:p:")) != -1){ 
        switch(opt) {
            case 'h':
                hostname = optarg;break;
            case 'u':
                username = optarg;break;
            case 'p':
                port = optarg;break;
            default:
                std::cerr << "Invalid Command Line Argument\n";
        }
    }

    std::cout << "$hostname: "<< hostname << "\nusername: " << username << "\nport: " << port << std::endl;

    Client myc(hostname, username, port);
    // You MUST invoke "run_client" function to start business logic
    myc.run_client();

    return 0;
}

int Client::connectTo()
{
	// ------------------------------------------------------------
    // In this function, you are supposed to create a stub so that
    // you call service methods in the processCommand/porcessTimeline
    // functions. That is, the stub should be accessible when you want
    // to call any service methods in those functions.
    // I recommend you to have the stub as
    // a member variable in your own Client class.
    // Please refer to gRpc tutorial how to create a stub.
	// ------------------------------------------------------------
    std::string target = hostname + ":" + port; 

    stub_ = TinySocialMedia::NewStub(grpc::CreateChannel(target, grpc::InsecureChannelCredentials()));

    ClientContext dumyContect_;

    UserName name;
    name.set_name(username);

    // Container for the data we expect from the server.
    UserId userId;

    // The actual RPC.
    Status status = stub_->joinServer(&dumyContect_, name, &userId);

    // Act upon its status.
    if (status.ok()) 
    {
      // return userInDataBase.existing(); 
      myId_ = userId.id();
      std::cout << "target: "<< target << "\nusername: " << username << "\nindex:" << myId_ << std::endl;
      return 1; // return 1 if success, otherwise return -1
    } 

    return -1;//"RPC failed";
}

IReply Client::processCommand(std::string& input)
{
	// ------------------------------------------------------------
	// GUIDE 1:
	// In this function, you are supposed to parse the given input
    // command and create your own message so that you call an 
    // appropriate service method. The input command will be one
    // of the followings:
	//
	// FOLLOW <username>
	// UNFOLLOW <username>
	// LIST
    // TIMELINE
	//
	// - JOIN/LEAVE and "<username>" are separated by one space.
	// ------------------------------------------------------------
	
    // ------------------------------------------------------------
	// GUIDE 2:
	// Then, you should create a variable of IReply structure
	// provided by the client.h and initialize it according to
	// the result. Finally you can finish this function by returning
    // the IReply.
	// ------------------------------------------------------------
    
	// ------------------------------------------------------------
    // HINT: How to set the IReply?
    // Suppose you have "Join" service method for JOIN command,
    // IReply can be set as follow:
    // 
    //     // some codes for creating/initializing parameters for
    //     // service method
    //     IReply ire;
    //     grpc::Status status = stub_->Join(&context, /* some parameters */);
    //     ire.grpc_status = status;
    //     if (status.ok()) {
    //         ire.comm_status = SUCCESS;
    //     } else {
    //         ire.comm_status = FAILURE_NOT_EXISTS;
    //     }
    //      
    //      return ire;
    // 
    // IMPORTANT: 
    // For the command "LIST", you should set both "all_users" and 
    // "following_users" member variable of IReply.
	// ------------------------------------------------------------
    

    IReply iReplay_;

    toUpperCaseWithLimit(input, 1); // Capitalizing the first charactor

    switch (input[0]) // checking the first letter
    {
        case 'F': // check for F in FOLLOW
            iReplay_ = processFollow(input);
            break;
        case 'U': // check for U in UNFOLLOW
            iReplay_ = processUnfollow(input);
            break;
        case 'L': // check for L in LIST
            iReplay_ = processList(input);
            break;
        case 'T': // check for L in LIST
            iReplay_ = processTimeline(input);
            break;
        default: 
            iReplay_.comm_status = FAILURE_INVALID;
            break;
    } // end of switch

    return iReplay_;
}



bool grpcFlag = false;
void Client::processTimeline()
{
	// ------------------------------------------------------------
    // In this function, you are supposed to get into timeline mode.
    // You may need to call a service method to communicate with
    // the server. Use getPostMessage/displayPostMessage functions
    // for both getting and displaying messages in timeline mode.
    // You should use them as you did in hw1.
	// ------------------------------------------------------------

    // ------------------------------------------------------------
    // IMPORTANT NOTICE:
    //
    // Once a user enter to timeline mode , there is no way
    // to command mode. You don't have to worry about this situation,
    // and you can terminate the client program by pressing
    // CTRL-C (SIGINT)
	// ------------------------------------------------------------
    ClientContext dumyContect_;

    std::shared_ptr<ClientReaderWriter<ClientPost, ServerPost>> stream(stub_->timeline(&dumyContect_));

    int clientId = myId_;


    // lets identidy ourselves
    ClientPost clientPost_;
    clientPost_.set_sender(clientId);
    clientPost_.set_content("Hi");
    stream->Write(clientPost_);

    grpcFlag = true;
    

    std::thread writer([stream, clientId]() 
    {
        while (grpcFlag) 
        {
            std::string content_ = getPostMessage();

            ClientPost clientPost_;
            clientPost_.set_sender(clientId);
            clientPost_.set_content(content_);

            stream->Write(clientPost_);
        }
        stream->WritesDone();
    });

    std::thread reader([stream]() 
    {
        ServerPost incommingPost_;
        while( stream->Read(&incommingPost_) )
        {
            std::string sender_ = incommingPost_.sendername();
            std::string content_ = incommingPost_.content();
            time_t time_ = static_cast<time_t> (incommingPost_.time());
           displayPostMessage(sender_, content_, time_);
        }
    });
    
    reader.join();
    grpcFlag = false;
    writer.join();
    
}



IReply Client::processFollow(std::string& input)
{
    IReply iReply_;
    if (doStringsMatchCaseInsensitive(&input[1], &COMMAND_FOLLOW[1])) // Checking OLLOW in FOLLOW
    {
        std::string userName = extractUserName(input, COMMAND_FOLLOW);
        if (userName.size()> 0) // making sure name is valid
        {
            ClientContext dumyContect_;

            FollowCommand grpcFollowCommand_;
            GeneralStatus generalStatus_;

            grpcFollowCommand_.set_sender(myId_);
            grpcFollowCommand_.set_nametofollow(userName);

            grpc::Status status = stub_->follow(&dumyContect_, grpcFollowCommand_, &generalStatus_);

            iReply_.comm_status = static_cast<IStatus>(generalStatus_.status());
            iReply_.grpc_status = status;

            return iReply_;
        }
    }

    iReply_.comm_status = FAILURE_INVALID;
    return iReply_;
}





IReply Client::processUnfollow(std::string& input)
{
    IReply iReply_;
    if (doStringsMatchCaseInsensitive(&input[1], &COMMAND_UNFOLLOW[1])) // Checking NFOLLOW in UNFOLLOW
    {
        std::string userName = extractUserName(input, COMMAND_UNFOLLOW);
        if (userName.size()> 0) // making sure name is valid
        {
            ClientContext dumyContect_;

            UnfollowCommand grpcUnfollowCommand_;
            GeneralStatus generalStatus_;

            grpcUnfollowCommand_.set_sender(myId_);
            grpcUnfollowCommand_.set_nametounfollow(userName);

            grpc::Status status = stub_->unfollow(&dumyContect_, grpcUnfollowCommand_, &generalStatus_);

            iReply_.comm_status = static_cast<IStatus>(generalStatus_.status());
            iReply_.grpc_status = status;

            return iReply_;
        }
    }
    
    iReply_.comm_status = FAILURE_INVALID;
    return iReply_;
}





IReply Client::processList(std::string& input)
{
    IReply iReply_;
    if (input.size() == COMMAND_LIST.size())
    {
        if (doStringsMatchCaseInsensitive(&input[1], &COMMAND_LIST[1])) // Checking IST in LIST
        {
            ClientContext dumyContect_;

            UserId userId_;
            ListReply listReply_;

            userId_.set_id(myId_);

            grpc::Status status = stub_->listUsers(&dumyContect_, userId_, &listReply_);

            for (int i = 0; i < listReply_.users_size(); i++)
            {
                std::string temp = listReply_.users(i);
                iReply_.all_users.push_back(temp);
            }

            for (int i = 0; i < listReply_.followings_size(); i++)
            {
                std::string temp = listReply_.followings(i);
                iReply_.followers.push_back(temp);
            }

            iReply_.comm_status = static_cast<IStatus>(listReply_.status());
            iReply_.grpc_status = status;
            
            return iReply_;
        }
    }
    iReply_.comm_status = FAILURE_INVALID;
    return iReply_;
}



IReply Client::processTimeline(std::string& input)
{
    IReply iReply_;
    if (input.size() == COMMAND_TIMELINE.size())
    {
        if (doStringsMatchCaseInsensitive(&input[1], &COMMAND_TIMELINE[1])) // Checking IST in LIST
        {
            iReply_.comm_status = SUCCESS;// static_cast<IStatus>(listReply_.status());
            iReply_.grpc_status = grpc::Status::OK;

            return iReply_;
        }
    }
    iReply_.comm_status = FAILURE_INVALID;
    return iReply_;
}