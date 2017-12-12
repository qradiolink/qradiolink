#include "channel.h"

Channel::Channel(int id, int parent_id, QString name, QString description, QObject *parent) :
    QObject(parent)
{
    this->id = id;
    this->parent_id = parent_id;
    this->name = name;
    this->description = description;
}
