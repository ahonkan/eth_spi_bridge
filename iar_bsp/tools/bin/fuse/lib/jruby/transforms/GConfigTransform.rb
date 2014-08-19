require 'rbconfig'
require 'java'

Node = javax.swing.tree.DefaultMutableTreeNode
TreeSelectionModel = javax.swing.tree.TreeSelectionModel
JFrame = javax.swing.JFrame
JPanel = javax.swing.JPanel
JScrollPane = javax.swing.JScrollPane
JSplitPane = javax.swing.JSplitPane
JEditorPane = javax.swing.JEditorPane
TreeSelectionListener = javax.swing.event.TreeSelectionListener

class NodeInfo
   def initialize(obj)
      @obj = obj
   end

   def node_obj
      @obj
   end

   def to_s
      @obj.getName()
   end
end

class NodeSelection
   include TreeSelectionListener

   def initialize(tree, editorPane)
      super()
      @tree = tree
      @editorPane = editorPane
   end

   def valueChanged(e)
      node = @tree.getLastSelectedPathComponent()
      if node != nil
         obj = node.getUserObject.node_obj
         text = ""

         if obj.respond_to? :getOptions
            text = ""
            obj.getOptions.each { |opt|
               text += opt.getName + " = " + opt.get.to_s + "\n"
            }
         end

         text += "dir = " + obj.getDirectory.to_s + "\n"
         text += "enabled = " + obj.getEnabled.to_s

         @editorPane.setText(text)
      end
   end
end

class GConfigTransform < Transform
   def initialize(phaseList)
      super(phaseList)
      @node_stack = []
   end
   
   def executePhase(context)
      @context = context
      context.getUniverse().accept(self)
      display()
      return true
   end
   
   def preVisitPackage(p)
      node = Node.new(p.getName())
      node.setUserObject(NodeInfo.new(p))
      if @node_stack[-1] != nil
         @node_stack[-1].add(node)
      end
      @node_stack.push(node)
   end

   def postVisitPackage(p)
        if @node_stack.length > 1
           @node_stack.pop
        end
   end
 
   def preVisitComponent(c)
      @base_directory = c.getDirectory();
      node = Node.new(c.getName())
      node.setUserObject(NodeInfo.new(c))
      @node_stack[-1].add(node)
   end
   
   def mouseClicked(me)
   end

   def display
      editorPane = JEditorPane.new
      editorPane.setEditable(false)

      tree = javax.swing.JTree.new(@node_stack[-1])
      tree.getSelectionModel().setSelectionMode(
         TreeSelectionModel.SINGLE_TREE_SELECTION)
      tree.addTreeSelectionListener(NodeSelection.new(tree, editorPane))

      treeView = JScrollPane.new(tree)
     
      splitPane = JSplitPane.new(JSplitPane::HORIZONTAL_SPLIT)
      splitPane.setLeftComponent(treeView)
      splitPane.setRightComponent(editorPane)
      splitPane.setResizeWeight(0.5)
      splitPane.setOneTouchExpandable(true)
      splitPane.setContinuousLayout(true)

      frame = JFrame.new("Nucleus Configuration")
      frame.getContentPane.add(splitPane)
      frame.setDefaultCloseOperation(JFrame::EXIT_ON_CLOSE)
      frame.pack
      frame.setVisible(true)
  end
end
