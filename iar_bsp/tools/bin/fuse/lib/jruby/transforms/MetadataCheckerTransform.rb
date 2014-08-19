class MetadataCheckerTransform < Transform
   def initialize(phaseList)
      super(phaseList)
      @namespace = []
      @duplicate_count = 0
   end
   
   def executePhase(context)
      @context = context
      # this visits everything in the universe
      context.getUniverse().accept(self)
      puts "Duplicate component / packages: " + @duplicate_count.to_s 
      return true
   end

  def preVisitPackage(p)
    @namespace.push("P(" + p.getName() + ")")
    p.getPackages().each { |subpackage|
      comp = p.findComponent(subpackage.getName())
      if comp != nil
        puts "Duplicate: " + subpackage.getName() + " at: "
        print_dot_name()
        puts comp.getDirectory()
        @duplicate_count += 1
      end
    }
    if not isNameValid?(p.getName())
      puts p.getName() + " contains a \"-\", \" \", or \".\". Please rename package."
    end
  end
  
  def postVisitPackage(p)
    @namespace.pop
  end
  
  def preVisitComponent(c)
    @namespace.push("C(" + c.getName() + ")")
    if c.getLibraries().length == 0 and c.getExecutables().length == 0
      puts @namespace.join(".") + "has no libraries or executables. Consider making it a package."
    end
    if not isNameValid?(c.getName())
      puts c.getName() + " contains a \"-\", \" \", or \".\". Please rename package."
    end
    
    #print_dot_name()
  end
  
  def postVisitComponent(c)
    @namespace.pop()
  end

  def print_dot_name
    puts @namespace.join(".")
  end
  
  def isNameValid?(s)
    if s.include?("-") or s.include?(" ") or s.include?(".")
      false
    else
      true
    end
  end
  
end