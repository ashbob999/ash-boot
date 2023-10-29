
template<class F>
class ScopeExit
{
public:
	explicit ScopeExit(F&& func) : function{ func } {}
	~ScopeExit() { function(); }

	ScopeExit(const ScopeExit&) = delete;
	ScopeExit& operator=(const ScopeExit&) = delete;

	ScopeExit(ScopeExit&& other) : function{ other.function } {}

private:
	F function;
};
