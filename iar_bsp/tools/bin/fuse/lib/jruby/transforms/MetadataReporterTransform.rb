class MetadataReporterTransform < Transform
   def initialize(phaseList)
      super(phaseList)
      @namespace = []
   end
   
   def executePhase(context)
      @context = context
      # this visits everything in the universe
      context.getUniverse().accept(self)
      return true
   end

  def preVisitPackage(p)
    @namespace.push("P(" + p.getName() + ")")
  end
  
  def postVisitPackage(p)
    @namespace.pop
  end
  
  def preVisitComponent(c)
    @namespace.push("C(" + c.getName() + ")")
    print_dot_name()
  end
  
  def postVisitComponent(c)
    @namespace.pop()
  end
  
  def preVisitOption(o)
    @namespace.push("O(" + o.getName() + ")")
    print_dot_name()
  end

  def postVisitOption(o)
    @namespace.pop()
  end
  
  def preVisitOptionGroup(g)
    @namespace.push("G("+ g.getName() + ")")
    print_dot_name()
  end
  
  def postVisitOptionGroup(g)
    @namespace.pop()
  end
  def print_dot_name
    puts @namespace.join(".")
  end
end