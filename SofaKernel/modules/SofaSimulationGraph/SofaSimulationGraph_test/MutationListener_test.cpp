#include <SceneCreator/SceneCreator.h>

#include <SofaTest/Sofa_test.h>

#include <SofaSimulationGraph/DAGSimulation.h>
#include <SofaSimulationGraph/DAGNode.h>
#include <sofa/simulation/Simulation.h>
#include <sofa/simulation/MutationListener.h>
using sofa::simulation::MutationListener;
using sofa::core::objectmodel::BaseObject;
using sofa::simulation::Simulation;
using sofa::simulation::Node;
using sofa::simulation::graph::DAGNode;
using sofa::modeling::addNew;



class TestMutationListener : public MutationListener
{
    void doAddChildBegin(Node* parent, Node* child)
    {
        log += "Begin Add " + child->getName() + " to " + parent->getName() + "\n";
    }
    void doAddChildEnd(Node* parent, Node* child)
    {
        log += "End Add " + child->getName() + " to " + parent->getName() + "\n";
    }

    void doRemoveChildBegin(Node* parent, Node* child)
    {
        log += "Begin Remove " + child->getName() + " from " + parent->getName() + "\n";
    }
    void doRemoveChildEnd(Node* parent, Node* child)
    {
        log += "End Remove " + child->getName() + " from " + parent->getName() + "\n";
    }

    void doAddObjectBegin(Node* parent, BaseObject* obj)
    {
        log += "Begin Add " + obj->getName() + " to " + parent->getName() + "\n";
    }
    void doAddObjectEnd(Node* parent, BaseObject* obj)
    {
        log += "End Add " + obj->getName() + " to " + parent->getName() + "\n";
    }

    void doRemoveObjectBegin(Node* parent, BaseObject* obj)
    {
        log += "Begin Remove " + obj->getName() + " from " + parent->getName() + "\n";
    }
    void doRemoveObjectEnd(Node* parent, BaseObject* obj)
    {
        log += "End Remove " + obj->getName() + " from " + parent->getName() + "\n";
    }

public:
    std::string log;
    void clearLog()
    {
        log = "";
    }
};

class MyComponent : public BaseObject {};

struct MutationListener_test : public sofa::BaseTest
{
    TestMutationListener listener;
    Node::SPtr root;
    Node::SPtr node1;
    Node::SPtr node1_1;
    Node::SPtr node1_2;
    Node::SPtr node2;
    BaseObject::SPtr obj1;
    BaseObject::SPtr obj2;


    MutationListener_test() {}

    /*TESTS CHILD*/
    void test_createChild()
    {
        node1 = root->createChild("node1");
        EXPECT_STREQ("Begin Add node1 to root\nEnd Add node1 to root\n", listener.log.c_str());
        listener.clearLog();
        node2 = root->createChild("node2");
        EXPECT_STREQ("Begin Add node2 to root\nEnd Add node2 to root\n", listener.log.c_str());
        listener.clearLog();
    }
    void test_addSubChild()
    {
        test_createChild();
        node1_1 = node1->createChild("node1_1");
        EXPECT_STREQ("Begin Add node1_1 to node1\nEnd Add node1_1 to node1\n", listener.log.c_str());
        listener.clearLog();
    }
    void test_addChild()
    {
        test_createChild();
        node1->addChild(node2);
        EXPECT_STREQ("Begin Add node2 to node1\nEnd Add node2 to node1\n", listener.log.c_str());
        listener.clearLog();
    }

    void test_removeSubChild()
    {
        test_addSubChild();
        node1->removeChild(node1_1);
        EXPECT_STREQ("Begin Remove node1_1 from node1\nEnd Remove node1_1 from node1\n", listener.log.c_str());
        listener.clearLog();
    }

    void test_removeChild()
    {
        test_removeSubChild();
        root->removeChild(node1);
        EXPECT_STREQ("Begin Remove node1 from root\nEnd Remove node1 from root\n", listener.log.c_str());
        listener.clearLog();
    }

    void test_removeNonExistingChild()
    {
        test_removeChild();
        root->removeChild(node1);
        EXPECT_STREQ("", listener.log.c_str());
        listener.clearLog();
    }

    // If the node is detached from the root node. In the previous impl,
    // removeChild would check if root has a child named node1, and since it's
    // been previously detached, would not do anything & would not notify the
    // listener.
    // In the new impl, delegates mask the impl of removeChild, and there's no
    // way to know if the removal will happen effectively. The
    // notifyRemoveChild will be called even if the node is detached.
    void test_removeDetachedChild()
    {
        test_removeChild();
        root->removeChild(node1);
        EXPECT_STREQ("", listener.log.c_str());
        listener.clearLog();
    }

    // A Move is a succession of 0-N Removes and 1 add.
    void test_moveChild()
    {
        node1 = root->createChild("node1");
        node2 = root->createChild("node2");
        node1_1 = node1->createChild("node1_1");
        listener.clearLog();
        node2->moveChild(node1_1);
        // New version considers a Move to be a succession of 0-N Removes and 1 add.
        // Thus no move message ever sent to the listener!
        EXPECT_STREQ(/*"Begin Move node1_1 from node1 to node2\n"*/
                     "Begin Remove node1_1 from node1\n"
                     "End Remove node1_1 from node1\n"
                     "Begin Add node1_1 to node2\n"
                     "End Add node1_1 to node2\n"
                     /*"End Move node1_1 from node1 to node2\n"*/, listener.log.c_str());
        listener.clearLog();
    }

    void test_moveChild2Parents()
    {
        node1 = root->createChild("node1");
        node2 = root->createChild("node2");
        node1_1 = node1->createChild("node1_1");
        node2->addChild(node1_1);
        listener.clearLog();
        root->moveChild(node1_1);
        // New version considers a Move to be a succession of 0-N Removes and 1 add.
        // Thus no move message ever sent to the listener!
        EXPECT_STREQ(/*"Begin Move node1_1 from node1 to node2\n"*/
                     "Begin Remove node1_1 from node1\n"
                     "End Remove node1_1 from node1\n"
                     "Begin Remove node1_1 from node2\n"
                     "End Remove node1_1 from node2\n"
                     "Begin Add node1_1 to root\n"
                     "End Add node1_1 to root\n"
                     /*"End Move node1_1 from node1 to node2\n"*/, listener.log.c_str());
        listener.clearLog();
    }

    void test_moveChildNoParent()
    {
        node1 = root->createChild("node1");
        node2 = root->createChild("node2");
        node1_1 = node1->createChild("node1_1");
        node1_1->detachFromGraph();
        listener.clearLog();
        root->moveChild(node1_1);
        // New version considers a Move to be a succession of 0-N Removes and 1 add.
        // Thus no move message ever sent to the listener!
        EXPECT_STREQ(/*"Begin Move node1_1 from node1 to node2\n"*/
                     "Begin Remove node1_1 from node1\n"
                     "End Remove node1_1 from node1\n"
                     "Begin Remove node1_1 from node2\n"
                     "End Remove node1_1 from node2\n"
                     "Begin Add node1_1 to root\n"
                     "End Add node1_1 to root\n"
                     /*"End Move node1_1 from node1 to node2\n"*/, listener.log.c_str());
        listener.clearLog();
    }

    /*TESTS OBJECT*/
    void test_addObject()
    {
        sofa::core::objectmodel::BaseObjectDescription bod1("obj1", "BaseObject");
        obj1 = sofa::core::objectmodel::New<BaseObject>();
        obj1->parse(&bod1);
        root->addObject(obj1);
        EXPECT_EQ("Begin Add obj1 to root\nEnd Add obj1 to root\n", listener.log);
        listener.clearLog();
        sofa::core::objectmodel::BaseObjectDescription bod2("obj2", "BaseObject");
        obj2 = sofa::core::objectmodel::New<BaseObject>();
        obj2->parse(&bod2);
        root->addObject(obj2);
        EXPECT_EQ("Begin Add obj2 to root\nEnd Add obj2 to root\n", listener.log);
        listener.clearLog();
    }

    void test_removeObject()
    {
        test_addObject();
        root->removeObject(obj1);
        EXPECT_EQ("Begin Remove obj1 from root\nEnd Remove obj1 from root\n", listener.log);
        listener.clearLog();
        root->removeObject(obj2);
        EXPECT_EQ("Begin Remove obj2 from root\nEnd Remove obj2 from root\n", listener.log);
        listener.clearLog();
    }

    // IMO this test should not pass, because it doesn't make sense to notify the
    // listener if the object is not in the node. But the delegate approach
    // prevents the check from happening
    void test_removeNonExistingObject()
    {
        test_removeObject();
        listener.clearLog();
        root->removeObject(obj1);
        EXPECT_EQ("Begin Remove obj1 from root\nEnd Remove obj1 from root\n", listener.log);
        listener.clearLog();
        root->removeObject(obj2);
        EXPECT_EQ("Begin Remove obj2 from root\nEnd Remove obj2 from root\n", listener.log);
        listener.clearLog();
    }

    void test_moveObject()
    {
        test_addObject();
        test_addChild();
        listener.clearLog();

        // New version considers a Move to be a succession of 0-N Removes and 1 add.
        // Thus no move message ever sent to the listener!
        node1->moveObject(obj1);
        EXPECT_EQ("Begin Remove obj1 from root\n"
                  "End Remove obj1 from root\n"
                  "Begin Add obj1 to node1\n"
                  "End Add obj1 to node1\n"
                  , listener.log);
        listener.clearLog();
        node1->moveObject(obj2);
        EXPECT_EQ("Begin Remove obj2 from root\n"
                  "End Remove obj2 from root\n"
                  "Begin Add obj2 to node1\n"
                  "End Add obj2 to node1\n"
                  , listener.log);
        listener.clearLog();
    }


    void test_addChildWithDescendency()
    {
        // Adding a child notifies recursively to every subnode / subObjects
        // present downstream
        DAGNode::SPtr node1 = sofa::core::objectmodel::New<DAGNode>("node1");
        DAGNode::SPtr node2 = sofa::core::objectmodel::New<DAGNode>("node2");
        node1->addChild(node2);
        sofa::core::objectmodel::BaseObjectDescription bod1("obj1", "BaseObject");
        obj1 = sofa::core::objectmodel::New<BaseObject>();
        obj1->parse(&bod1);
        node2->addObject(obj1);
        sofa::core::objectmodel::BaseObjectDescription bod2("obj2", "BaseObject");
        obj2 = sofa::core::objectmodel::New<BaseObject>();
        obj2->parse(&bod2);
        node2->addObject(obj2);
        listener.clearLog();

        root->addChild(node1);
        EXPECT_EQ("Begin Add node1 to root\n"
                  "Begin Add node2 to node1\n"
                  "Begin Add obj1 to node2\n"
                  "Begin Add obj2 to node2\n"

                  "End Add node1 to root\n"
                  "End Add node2 to node1\n"
                  "End Add obj1 to node2\n"
                  "End Add obj2 to node2\n",
                  listener.log);
        listener.clearLog();
    }

    void test_removeChildWithDescendency()
    {
        // Removing a child does NOT notify recursively for every
        // subnode / subObjects present downstream
        DAGNode::SPtr node1 = sofa::core::objectmodel::New<DAGNode>("node1");
        DAGNode::SPtr node2 = sofa::core::objectmodel::New<DAGNode>("node2");
        node1->addChild(node2);
        sofa::core::objectmodel::BaseObjectDescription bod1("obj1", "BaseObject");
        obj1 = sofa::core::objectmodel::New<BaseObject>();
        obj1->parse(&bod1);
        node2->addObject(obj1);
        sofa::core::objectmodel::BaseObjectDescription bod2("obj2", "BaseObject");
        obj2 = sofa::core::objectmodel::New<BaseObject>();
        obj2->parse(&bod2);
        node2->addObject(obj2);
        root->addChild(node1);
        listener.clearLog();

        root->removeChild(node1);
        EXPECT_EQ("Begin Remove node1 from root\n"
                  "End Remove node1 from root\n",
                  listener.log);
        listener.clearLog();
    }

    void test_moveChildWithDescendency()
    {
        // Moving a child removes the node from its parent(s) and adds it
        // (recursively) to the new parent
        DAGNode::SPtr node1 = sofa::core::objectmodel::New<DAGNode>("node1");
        DAGNode::SPtr node2 = sofa::core::objectmodel::New<DAGNode>("node2");
        node1->addChild(node2);
        sofa::core::objectmodel::BaseObjectDescription bod1("obj1", "BaseObject");
        obj1 = sofa::core::objectmodel::New<BaseObject>();
        obj1->parse(&bod1);
        node2->addObject(obj1);
        sofa::core::objectmodel::BaseObjectDescription bod2("obj2", "BaseObject");
        obj2 = sofa::core::objectmodel::New<BaseObject>();
        obj2->parse(&bod2);
        node2->addObject(obj2);
        root->addChild(node1);
        Node::SPtr node3 = root->createChild("node3");
        listener.clearLog();

        node3->moveChild(node1);
        EXPECT_EQ("Begin Remove node1 from root\n"
                  "End Remove node1 from root\n"
                  "Begin Add node1 to node3\n"
                  "Begin Add node2 to node1\n"
                  "Begin Add obj1 to node2\n"
                  "Begin Add obj2 to node2\n"
                  "End Add node1 to node3\n"
                  "End Add node2 to node1\n"
                  "End Add obj1 to node2\n"
                  "End Add obj2 to node2\n",
                  listener.log);
        listener.clearLog();
    }

    void test_moveChildWithDescendencyAndMultipleParents()
    {
        DAGNode::SPtr node1 = sofa::core::objectmodel::New<DAGNode>("node1");
        DAGNode::SPtr node2 = sofa::core::objectmodel::New<DAGNode>("node2");
        DAGNode::SPtr node3 = sofa::core::objectmodel::New<DAGNode>("node3");
        DAGNode::SPtr node4 = sofa::core::objectmodel::New<DAGNode>("node4");
        DAGNode::SPtr node5 = sofa::core::objectmodel::New<DAGNode>("node5");
        DAGNode::SPtr node6 = sofa::core::objectmodel::New<DAGNode>("node6");
        DAGNode::SPtr node7 = sofa::core::objectmodel::New<DAGNode>("node7");
        DAGNode::SPtr node8 = sofa::core::objectmodel::New<DAGNode>("node8");

        sofa::core::objectmodel::BaseObjectDescription bod1("obj1", "BaseObject");
        obj1 = sofa::core::objectmodel::New<BaseObject>();
        obj1->parse(&bod1);
        node1->addObject(obj1);
        sofa::core::objectmodel::BaseObjectDescription bod2("obj2", "BaseObject");
        obj2 = sofa::core::objectmodel::New<BaseObject>();
        obj2->parse(&bod2);
        node1->addObject(obj2);

        root->addChild(node2);
        root->addChild(node3);
        root->addChild(node4);
        root->addChild(node5);
        root->addChild(node6);
        root->addChild(node7);
        root->addChild(node8);

        node2->addChild(node1);
        node3->addChild(node1);
        node4->addChild(node1);
        listener.clearLog();

        root->moveChild(node1);
        EXPECT_EQ("Begin Remove node1 from node2\n"
                  "End Remove node1 from node2\n"
                  "Begin Remove node1 from node3\n"
                  "End Remove node1 from node3\n"
                  "Begin Remove node1 from node4\n"
                  "End Remove node1 from node4\n"
                  "Begin Add node1 to root\n"
                  "Begin Add obj1 to node1\n"
                  "Begin Add obj2 to node1\n"
                  "End Add node1 to root\n"
                  "End Add obj1 to node1\n"
                  "End Add obj2 to node1\n",
                  listener.log);
    }

    void SetUp()
    {
        sofa::simulation::Simulation* simu;
        sofa::simulation::setSimulation(
                    simu = new sofa::simulation::graph::DAGSimulation());

        root = simu->createNewGraph("root");
        root->addListener(&listener);
    }
    void TearDown()
    {
    }
};

TEST_F(MutationListener_test, test_createChild) { test_createChild(); }
TEST_F(MutationListener_test, test_addChild) { test_addChild(); }
TEST_F(MutationListener_test, test_addSubChild){ test_addSubChild(); }
TEST_F(MutationListener_test, test_removeSubChild){ test_removeSubChild(); }
TEST_F(MutationListener_test, test_removeChild){ test_removeChild(); }
TEST_F(MutationListener_test, test_removeNonExistingChild){ test_removeNonExistingChild(); }
TEST_F(MutationListener_test, test_removeDetachedChild){ test_removeDetachedChild(); }
TEST_F(MutationListener_test, test_moveChild) { test_moveChild(); }

TEST_F(MutationListener_test, test_addObject) { test_addObject(); }
TEST_F(MutationListener_test, test_removeObject) { test_removeObject(); }
TEST_F(MutationListener_test, test_removeNonExistingObject){ test_removeNonExistingObject(); }
TEST_F(MutationListener_test, test_moveObject) { test_moveObject(); }

TEST_F(MutationListener_test, test_addChildWithDescendency) { test_addChildWithDescendency(); }
TEST_F(MutationListener_test, test_removeChildWithDescendency) { test_removeChildWithDescendency(); }
TEST_F(MutationListener_test, test_moveChildWithDescendency) { test_moveChildWithDescendency(); }
TEST_F(MutationListener_test, test_moveChildWithDescendencyAndMultipleParents) { test_moveChildWithDescendencyAndMultipleParents(); }
