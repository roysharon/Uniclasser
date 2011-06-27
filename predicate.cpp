// Copyright (c) 2010 Roy Sharon <roy@roysharon.com>
// See project repositry at <https://github.com/roysharon/Uniclasser>
// Using this file is subject to the MIT License <http://creativecommons.org/licenses/MIT/>

#include <cassert>
#include "predicate.hpp"

//----- Predicate -------------------------------------------------------------

bool Predicate::push(IPredicate *p)
{
	if (predicate == 0) predicate = p;
	else return predicate->push(p);
	return true;
}

void Predicate::accept(IGenerator &generator)
{
	assert(!push(0)); // Cannot generate an incomplete predicate
	
	generator.visit(*predicate);
}
	

//----- TerminalPredicate -----------------------------------------------------

void TerminalPredicate::accept(IGenerator &generator)
{
	generator.visit(*this);
}
	

//----- AndPredicate ----------------------------------------------------------

bool AndPredicate::push(IPredicate *p)
{
	if (complete) return false;
	else if (lhs == 0) lhs = p;
	else if (!lhs->push(p))
	{
		if (rhs == 0) rhs = p;
		else if (!rhs->push(p))
		{
			complete = true;
			return false;
		}
	}
	return true;
}

void AndPredicate::accept(IGenerator &generator)
{
	generator.visit(*this);
}
	

//----- OrPredicate -----------------------------------------------------------

bool OrPredicate::push(IPredicate *p)
{
	if (complete) return false;
	else if (lhs == 0) lhs = p;
	else if (!lhs->push(p))
	{
		if (rhs == 0) rhs = p;
		else if (!rhs->push(p))
		{
			complete = true;
			return false;
		}
	}
	return true;
}

void OrPredicate::accept(IGenerator &generator)
{
	generator.visit(*this);
}


//----- TernaryPredicate ------------------------------------------------------

bool TernaryPredicate::push(IPredicate *p)
{
	if (complete) return false;
	else if (predicate == 0) predicate = p;
	else if (!predicate->push(p))
	{
		if (off == 0) off = p;
		else if (!off->push(p))
		{
			if (on == 0) on = p;
			else if (!on->push(p))
			{
				complete = true;
				return false;
			}
		}
	}
	return true;
}

void TernaryPredicate::accept(IGenerator &generator)
{
	generator.visit(*this);
}
