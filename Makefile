# Detect compiler and LLVM (adjust LLVM_CONFIG path if needed)
CXX        = c++
CXXFLAGS   = -std=c++17 -O2 -Wall
CPPFLAGS   = -I./inc $(shell $(LLVM_CONFIG) --cxxflags)
LDFLAGS    = $(shell $(LLVM_CONFIG) --ldflags)
LDLIBS     = $(shell $(LLVM_CONFIG) --libs --system-libs)

# Directories
SRCDIR     = src
OBJDIR     = obj
TARGET     = lr

# Source and object files
SRCS       = $(wildcard $(SRCDIR)/*.cpp)
OBJS       = $(SRCS:$(SRCDIR)/%.cpp=$(OBJDIR)/%.o)

# Link step – use the object files from OBJDIR
$(TARGET): $(OBJS)
	$(CXX) $(LDFLAGS) $^ $(LDLIBS) -o $@

# Automatically create obj directory if missing
$(OBJDIR):
	mkdir -p $(OBJDIR)

# Compile step – create obj directory then compile
$(OBJDIR)/%.o: $(SRCDIR)/%.cpp | $(OBJDIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJDIR) $(TARGET)

.PHONY: all clean
