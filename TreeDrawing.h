#pragma once

#include "gtypes.h"
#include "gwindow.h"
#include <functional>
#include <memory>
#include <string>

/* Type: Labeler
 *
 * A type representing a function that takes in a node and outputs
 * a string used to label that node in a drawing of the tree.
 */
template <typename NodeType>
using Labeler = std::function<std::string(NodeType*)>;

/* Utility type used in the constructor. This is basically the identity
 * function for types.
 */
namespace TreeDrawing_Utility {
    template <typename T> struct Identity {
        using result = T;
    };
};

class TreeDrawing {
public:
    /* Constructs a TreeDrawing that doesn't draw anything. */
    TreeDrawing() = default;

    /* Constructs a TreeDrawing of the given tree. Any type may be
     * used here as long as it has publicly-accessible fields named
     * 'left' and 'right' and uses nullptr to mark missing nodes.
     */
    template <typename NodeType>
    explicit TreeDrawing(NodeType* root);

    /* Constructs a TreeDrawing for the given tree that uses the provided
     * labeler.
     *
     * The second argument is obfuscated using the Identity type to make
     * sure that C++ doesn't try using it to deduce what NodeType is. Only
     * the first argument gives it the ability to do that. C++ is a
     * Wonderful Language that has no unusual interactions. >_<
     */
    template <typename NodeType>
    explicit TreeDrawing(NodeType* root, Labeler<typename TreeDrawing_Utility::Identity<NodeType>::result> labeler);

    /* Draws the tree at the given location within the indicated bounds. */
    void draw(GWindow& window, const GRectangle& bounds) const;
    void draw(GCanvas* canvas, const GRectangle& bounds) const;

private:
    /* Every node has unit diameter. */
    static const double kNodeRadius;

    /* Intended spacing between nodes on the same level. This is
     * the amount from the center of one node to the next. It's
     * two diameters of a node.
     */
    static const double kMinSeparation;

    /* Vertical spacing between nodes. */
    static const double kVerticalSpacing;

    /* Simpler representation of the tree that's fed as input to the actual
     * tree layout algorithm. This is done both to firewall off the .cpp from
     * the templates of the .h and to make things easier to read/maintain.
     * However, it's most definitely not necessary.
     */
    struct Node {
        std::shared_ptr<Node>        left;
        std::shared_ptr<Node>        right;
        std::function<std::string()> label;
        GPoint                       position;
    };

    /* Intermediate node structure for building a tree. */
    struct ThreadedNode {
        /* Left and right children. (We own our children.) */
        std::unique_ptr<ThreadedNode> leftChild;
        std::unique_ptr<ThreadedNode> rightChild;

        /* Next nodes along the left and right hulls of the tree. For nodes
         * that are on the actual hull of the tree, these will point to
         * the next node on the hull when moving in the indicated direction.
         * For nodes that are not on the actual hull of the tree, these values
         * will not be read.
         */
        ThreadedNode* leftHull  = nullptr;
        ThreadedNode* rightHull = nullptr;

        /* Signed distance from the node to the next nodes on the left/right
         * hull from this point.
         */
        double leftHullDistance  = -kMinSeparation / 2;
        double rightHullDistance =  kMinSeparation / 2;
    };

    /* Result from one level of the recursion. */
    struct ThreadedLayout {
        /* Pointer to tree root. */
        std::unique_ptr<ThreadedNode> root;

        /* Leftmost and rightmost leaves. */
        ThreadedNode* extremeLeft;
        double extremeLeftOffset;

        ThreadedNode* extremeRight;
        double extremeRightOffset;
    };

    /* Converts the original tree into our internal tree format. */
    template <typename NodeType> static std::shared_ptr<Node> convert(NodeType* root,
                                                                           Labeler<NodeType> labeler) {
        if (root == nullptr) return nullptr;

        return std::shared_ptr<Node>(new Node{
                                              convert(root->left,  labeler),
                                              convert(root->right, labeler),
                                              [=] () { return labeler(root); },
                                              {}
                                          });
    }

    /* Does the layout logic. */
    void performLayout();
    static ThreadedLayout layOutTree(Node* root);
    static void placeNodesIn(Node* inputRoot,
                             ThreadedNode* root,
                             double x = 0,
                             double y = 0);
    static GRectangle boundsFor(Node* root);

    /* Default labeler doesn't write anything. */
    template <typename NodeType> static std::string defaultLabeler(NodeType*) {
        return "";
    }

    /*** Data Members ***/

    /* Root of the reference tree. */
    std::shared_ptr<Node> root_;

    /* Radius of each node. */
    double nodeRadius_;

    /* Bounding box for all nodes. */
    GRectangle bounds_;
};

/* * * * * Implementation Below This POint * * * * */

template <typename NodeType>
TreeDrawing::TreeDrawing(NodeType* root) : TreeDrawing(root, defaultLabeler<NodeType>) {

}

template <typename NodeType>
TreeDrawing::TreeDrawing(NodeType* root, Labeler<typename TreeDrawing_Utility::Identity<NodeType>::result> labeler) {
    root_ = convert(root, labeler);
    performLayout();
}
