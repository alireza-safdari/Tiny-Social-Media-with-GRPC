/*
 *
 * Copyright 2015 gRPC authors.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <thread>
#include <chrono>
#include <unistd.h>

#include "tsm.grpc.pb.h"
#include <grpc++/grpc++.h>


using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using grpc::ServerReader;
using grpc::ServerReaderWriter;
using grpc::ServerWriter;

using tsm::UserName;
using tsm::UserId;
using tsm::TinySocialMedia;
using tsm::GeneralStatus;
using tsm::FollowCommand;
using tsm::UnfollowCommand;
using tsm::ListReply;
using tsm::ClientPost;
using tsm::ServerPost;


const int N_POST_IN_WALL = 20;

struct Client
{
  std::string name;
  std::vector<int64_t> followingIds;
  std::vector<int64_t> followerIds;
  bool isTimeLineMode;
  bool isThereNewPost;
  ServerPost newServerPost;
  std::vector<ServerPost> wallPosts;
};


std::vector<Client> clients;



int64_t clientLookUp(std::string& name)
{
  for (auto i = 0; i < clients.size(); i++)
  {
    if (clients[i].name == name)
    {
      return i;
    }
  }

  return -1;
}



const int64_t indexLookUp(const int64_t indexToCheck, std::vector<int64_t>& inputVector)
{
  for (int i = 0; i < inputVector.size(); i++)
  {
    if (inputVector[i] == indexToCheck)
      return i;
  }
  return -1;
}




bool isIndexUnique(const int64_t indexToCheck, std::vector<int64_t>& inputVector)
{
  for(int i = 0; i < inputVector.size(); i++)
  {
    if (inputVector[i] == indexToCheck)
      return false;
  }
  return true;
}




void updateWallPosts(const int64_t clientId, ServerPost serverPost) // place the new post in wall post
{
  if (clients[clientId].wallPosts.size() == N_POST_IN_WALL)
  {
    auto oldestPost = clients[clientId].wallPosts.begin();
    clients[clientId].wallPosts.erase(oldestPost);

    clients[clientId].wallPosts.push_back(serverPost);
  }
  else
  {
    clients[clientId].wallPosts.push_back(serverPost);
  }
}




// Logic and data behind the server's behavior.
class tsmServer final : public TinySocialMedia::Service {
  Status joinServer(ServerContext* context, const UserName* username,
                  UserId* userId) override {
    std::string name_ = username->name();

    int64_t id_ = clientLookUp(name_);

    if (id_ == -1)
    {
      Client newClient;

      id_ = clients.size();
      newClient.name = name_;
      newClient.isTimeLineMode = false;
      newClient.isThereNewPost = false; 
      newClient.followingIds.push_back(id_); // each user should follow itself
      newClient.followerIds.push_back(id_); // each user should follow itself
      clients.push_back(newClient);
      std::cout << "new user added: " << name_ << std::endl;
    }
    else
    {
      std::cout << "existing user connected: " << name_ << std::endl;
    }

    userId->set_id(id_);

    return Status::OK;
  }


  Status follow(ServerContext* context, const FollowCommand* followCommand,
                  GeneralStatus* status) override {
                    
    int64_t senderId_ = followCommand->sender();
    std::string name_ = followCommand->nametofollow();

    int64_t idToFollow_ = clientLookUp(name_);

    std::cout << "user:" << clients[senderId_].name << " (" << senderId_ << ")" 
              <<" want to follow:"<< name_ << " (" << idToFollow_ << ")";

    if ( idToFollow_ != -1 ) // username is Valid
    {
      if (isIndexUnique(idToFollow_, clients[senderId_].followingIds)) // making sure does not exist already
      {
        clients[senderId_].followingIds.push_back(idToFollow_); // adding to the following list of the user who called
        clients[idToFollow_].followerIds.push_back(senderId_); // adding to the follower list of the user being followed

        status->set_status(tsm::SUCCESS);
        std::cout << "--Done" << std::endl;

        return Status::OK; // you may remove this later, this is just for bug reporting
      }
      else
      {
        // user has been followed before
        status->set_status(tsm::FAILURE_ALREADY_EXISTS);
      }
    }
    else
    {
      // the username is not valid
      status->set_status(tsm::FAILURE_INVALID_USERNAME);
    }

    std::cout << "--Failed" << std::endl;
    return Status::OK;
  }




Status unfollow(ServerContext* context, const UnfollowCommand* unfollowCommand,
                  GeneralStatus* status) override {
                    
    int64_t senderId_ = unfollowCommand->sender();
    std::string name_ = unfollowCommand->nametounfollow();

    int64_t idToUnfollow_ = clientLookUp(name_);

    std::cout << "user:" << clients[senderId_].name << " (" << senderId_ << ")" 
              <<" want to unfollow:"<< name_ << " (" << idToUnfollow_ << ")";


    if ( idToUnfollow_ != -1 ) // user exist
    {
      if (senderId_ != idToUnfollow_) // making sure user does not add him/herself
      {
        int64_t indexToBeRemoved_ = indexLookUp(idToUnfollow_, clients[senderId_].followingIds);
        if (indexToBeRemoved_ != -1) // if the index is not in vector we do not need to remove it
        {
          // idToUnfollow_  exist in the vector we need to remove it
          auto whatShouldBeRemoved = clients[senderId_].followingIds.begin() + indexToBeRemoved_;
          clients[senderId_].followingIds.erase(whatShouldBeRemoved); 

          // now we need to remove sender from follower list of the 
          indexToBeRemoved_ = indexLookUp(senderId_, clients[idToUnfollow_].followerIds);
          whatShouldBeRemoved = clients[idToUnfollow_].followerIds.begin() + indexToBeRemoved_;
          clients[idToUnfollow_].followerIds.erase(whatShouldBeRemoved); 

          status->set_status(tsm::SUCCESS);
          std::cout << "--Done" << std::endl;

          return Status::OK; // you may remove this later, this is just for bug reporting
        }
        else
        {
          // user has been unfollowed before
          status->set_status(tsm::FAILURE_ALREADY_EXISTS);
        }
      }
      else
      {
          // user should not unfollow him/herself (meaningless)
          status->set_status(tsm::FAILURE_INVALID_USERNAME);
      }
    }
    else
    {
      // the username is not valid
      status->set_status(tsm::FAILURE_INVALID_USERNAME);
    }

    std::cout << "--Failed" << std::endl;

    return Status::OK;
  }



  Status listUsers(ServerContext* context, const UserId* userId,
                  ListReply* listReply) override
  {
    int64_t senderId_ = userId->id();

    listReply->set_status(tsm::SUCCESS);

    // name of all users
    for (int i = 0; i < clients.size(); i++)
    {
      listReply->add_users(clients[i].name);
    }

    // name of users the user follow
    for (int i = 0; i < clients[senderId_].followerIds.size() ; i++)
    {
      int64_t followerId = clients[senderId_].followerIds[i];
      std::string followerName = clients[followerId].name;
      listReply->add_followings(followerName);
    }

    return Status::OK;
  }




  Status timeline(ServerContext* context,
                   ServerReaderWriter<ServerPost, ClientPost>* stream) override 
  {
    ClientPost clientpost;
    
    //Hearing client hi
    stream->Read(&clientpost);
    int64_t clientId = clientpost.sender();
    clients[clientId].isTimeLineMode = true;


    for (int i = 0; i < clients[clientId].wallPosts.size() ; i++)
    {
      stream->Write(clients[clientId].wallPosts[i]);
    }


    std::thread writer([stream, clientId]() 
    {
      while(clients[clientId].isTimeLineMode == true)
      {
        if (clients[clientId].isThereNewPost) 
        {
          stream->Write(clients[clientId].newServerPost);
          clients[clientId].isThereNewPost = false;
        }
        std::this_thread::sleep_for (std::chrono::milliseconds(30));
      }
    });


    std::thread reader([stream]() 
    {
      ClientPost clientpost_;
      while ( stream->Read(&clientpost_) ) 
      {
        // for accurate timing we do time first
        std::time_t timeNow_ = std::time(nullptr);
        int32_t timeNumber_ = static_cast<int32_t> (timeNow_);
        
        // extract information
        int64_t clientId_ = clientpost_.sender();
        std::string content = clientpost_.content();

        // debugging
        std::string clientName = clients[clientId_].name;
        std::cout << "\"" << clientName << "\" said:" << content;

        ServerPost serverPost_;
        serverPost_.set_time(timeNumber_);
        serverPost_.set_sendername(clientName);
        serverPost_.set_content(content);

        updateWallPosts(clientId_, serverPost_);

        for(int i = 1; i < clients[clientId_].followerIds.size(); i++) // we start from 1 because 1 is user itself
        {
          int64_t idToSend = clients[clientId_].followerIds[i];
          if (clients[idToSend].isTimeLineMode)
          {
            while (clients[idToSend].isThereNewPost)
            {
              std::this_thread::sleep_for (std::chrono::milliseconds(10));
            }
            std::cout << "Telling \"" << clients[idToSend].name << "\" -- " ;
            clients[idToSend].newServerPost = serverPost_;
            clients[idToSend].isThereNewPost = true;
          }
          updateWallPosts(idToSend, serverPost_); // making sure the user know about it if he/she connected later
        }

        std::cout << std::endl;
      }

    });

    reader.join();
    clients[clientId].isTimeLineMode = false;
    writer.join();

    return Status::OK;
  }
};


void RunServer(std::string port) {

  std::string hostname = "localhost";
  std::string server_address = hostname + ":" + port; 

  tsmServer service;

  ServerBuilder builder;
  // Listen on the given address without any authentication mechanism.
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  // Register "service" as the instance through which we'll communicate with
  // clients. In this case it corresponds to an *synchronous* service.
  builder.RegisterService(&service);
  // Finally assemble the server.
  std::unique_ptr<Server> server(builder.BuildAndStart());
  std::cout << "Server listening on " << server_address << std::endl;

  // Wait for the server to shutdown. Note that some other thread must be
  // responsible for shutting down the server for this call to ever return.
  server->Wait();
}

int main(int argc, char** argv) {

  std::string port = "3010";

  int opt = 0;
  while ((opt = getopt(argc, argv, "p:")) != -1){ 
        switch(opt) {
            case 'p':
                port = optarg;break;
            default:
                std::cerr << "Invalid Command Line Argument\n";
        }
    }

  RunServer(port);

  return 0;
}
