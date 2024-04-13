#include "../include/server_logger_builder.h"
#include "../include/server_logger.h"
#include <mqueue.h>
#include <cstring>
#include <unistd.h>
#include <string>

struct queue_message
{
    std::string data_and_time;
    std::string text;
    std::string pid_queue;
    logger::severity severity_for_logger;
    size_t id;
};

int main(int argc, char *argv[])
{
    std::string file_name = "/queue_linux";

    struct mq_attr attrib_queue;
    attrib_queue.mq_flags = 0;
    attrib_queue.mq_maxmsg = 10;
    attrib_queue.mq_msgsize = 10;
    attrib_queue.mq_curmsgs = 0;

    mqd_t queue_1;

    queue_1 = mq_open(file_name.c_str(), O_CREAT | O_RDONLY, 0644, &attrib_queue);
    if (queue_1 == (mqd_t) - 1)
    {
        std::cout << "guten morgen!" << std::endl;
        std::cerr << "Error creating message queue. Errno: " << errno << std::endl;
        throw std::runtime_error("It is impossible to create a queue");
    }

    logger_builder* builder = new server_logger_builder();

    logger *logger = builder
            ->add_file_stream(file_name, logger::severity::information)
            ->add_file_stream(file_name, logger::severity::debug)
            ->build();

    delete builder;

    logger->log("66666666666666666666", logger::severity::information)
            ->log("eruygfquhbjnkigytfgvehbjkjqioy78t6cgvhbjioydrtcgvhbhudrtvgbhnj hugyfdrtcgvhjbhufdrcgvhjbhuiutfygvjhbhuuhuy uergf34pty743 5y72 348ty54uoeruhgfhoer y8753ythgyrgiwur8y429t5t", logger::severity::debug);

    mq_getattr(queue_1, &attrib_queue);

    size_t msg_size = attrib_queue.mq_msgsize;

    queue_message received_message;
    unsigned int priority;

    if (mq_receive(queue_1, reinterpret_cast<char*>(&received_message), msg_size, &priority) == -1)
    {
        throw std::runtime_error("Error receiving message from queue");
    }

    std::string message_received = "Received message: ";
//    message_received += "Severity: " + std::to_string(received_message.severity_for_logger) + ", ";
    message_received += "Date and Time: " + received_message.data_and_time + ", ";
    message_received += "PID: " + received_message.pid_queue + ", ";
    message_received += "Text: " + received_message.text + ", ";
    message_received += "ID: " + std::to_string(received_message.id);


    delete logger;
    mq_close(queue_1);
    mq_unlink(file_name.c_str());

    return 0;
}