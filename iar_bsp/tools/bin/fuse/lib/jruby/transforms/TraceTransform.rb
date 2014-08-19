class TraceTransform < Transform
   def initialize(phaseList)
      super(phaseList)
      @depth = 0
      @namespace = []
   end
   
   def executePhase(context)
      @context = context
      # this visits everything in the universe
      context.getUniverse().accept(self)

      return true
   end

  def preVisitPackage(p)
    @namespace[@depth] = p.getName()
    @depth += 1
    puts "  " * @depth + "P:" + p.getName()
  end
  
  def postVisitPackage(p)
    @depth -= 1;
  end
  
  def preVisitComponent(c)
    @depth += 1
    puts "  " * @depth + "C:" + c.getName()
  end
  
  def postVisitComponent(c)
    @depth -= 1
  end
end
