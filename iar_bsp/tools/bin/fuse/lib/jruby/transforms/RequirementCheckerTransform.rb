class RequirementException < Exception
end

class RequirementCheckerTransform < Transform
   def initialize(phaseList)
      super(phaseList)
      @namespace = []
   end
   
   def executePhase(context)
      @context = context
      begin
         context.getUniverse().accept(self)
         return true
      rescue RequirementException => e
         puts "error: " + e.message
      end
      return false
   end

   def preVisitPackage(p)
      if p.getName() != ''
         @namespace.push(p.getName())
      end
   end
  
   def postVisitPackage(p)
      if p.getName() != ''
         @namespace.pop
      end
   end
  
   def preVisitComponent(c)
      @namespace.push(c.getName())
      # if the component is enabled, test its requirements
      if c.getEnabled()
         c.getRequirements().each {|r|
            # r.requiredComponentName is required by this component
            required_c = @context.getUniverse().findFQComponent(
               r.getRequiredComponent())

            if required_c != nil
               if not required_c.getEnabled()
                  msg =  "requirement was not met: '#{@namespace.join(".")}' "
                  msg += "requires '#{r.getRequiredComponent}'"
                  msg += ", but '#{r.getRequiredComponent}' is disabled."
                  raise RequirementException, msg
               end
            else
               # Check if requirement specified is a package
               required_p = @context.getUniverse().findFQPackage(
                  r.getRequiredComponent())

               if required_p != nil
                  if not required_p.getEnabled()
                     msg =  "requirement was not met: '#{@namespace.join(".")}' "
                     msg += "requires '#{r.getRequiredComponent}'"
                     msg += ", but '#{r.getRequiredComponent}' is disabled."
                     raise RequirementException, msg
                  end
               else
                  msg =  "requirement was not met: '#{@namespace.join(".")}' "
                  msg += "requires '#{r.getRequiredComponent}'"
                  msg += ", but '#{r.getRequiredComponent}' does not exist."
                  raise RequirementException, msg
               end
            end
         }
      end
   end
  
   def postVisitComponent(c)
      @namespace.pop()
   end
end

