GRAPH_START = <<END
digraph %s {
ranksep=1;
rankdir=LR;
ratio=auto;
END

GRAPH_END = <<END
}
END

class GraphWriterTransform < Transform
   def initialize(phaseList)
      super(phaseList)
      @namespace = []
   end
   
   def executePhase(context)
      @context = context
      # this visits everything in the universe
      
      context.getUniverse().getPackages().each { |pack|
        puts GRAPH_START%pack.getName()
        pack.accept(self)
        puts GRAPH_END
      }
      return true
   end

  def preVisitPackage(p)
    @namespace.push(p.getName().gsub("-","_").gsub(" ","_"))
    puts @namespace.join("x") + " [label=\"" + p.getName().gsub("-","_").gsub(" ","_") + "\"]\;" 
    p.getPackages().each { |subpackage|
      puts @namespace.join("x") + " -> " + @namespace.join("x")+ "x" + subpackage.getName().gsub("-","_").gsub(" ","_") 

      
    }
    p.getComponents().each {|comp|
      puts @namespace.join("x") + " -> " + @namespace.join("x")+ "x" + comp.getName().gsub("-","_").gsub(" ","_")     
    }

  end
  
  def postVisitPackage(p)
   @namespace.pop
  end
  
  def preVisitComponent(c)
    @namespace.push(c.getName().gsub("-","_").gsub(" ","_"))
    puts @namespace.join("x") + " [label=\"" + c.getName().gsub("-","_").gsub(" ","_") + "\",shape=\"box\"]\;"    
    c.getRequirements().each {|r|
    required_c = @context.getUniverse().findFQComponent(r.getRequiredComponent())      
    if required_c != nil
      puts @namespace.join("x") + " -> " + r.getRequiredComponent().gsub(".","x") + "[ color=red, style=dotted]\;"       
    end
    }
  end
  
  def postVisitComponent(c)
    @namespace.pop()
  end

  def print_dot_name
    puts @namespace.join("->")
  end
  
end